#include "IOManager.h"
#include "Macro.h"
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#if 1

namespace DYX {

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(EVENT event){
    switch(event){
        case READ:
            return read_event;
        case WRITE:
            return write_event;
        default:
            MY_ASSERT2(false,"getcontext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext &context){
    context.cb = nullptr;
    context.fiber.reset();
    context.scheduler = nullptr;
}

//当某个 fd 上的事件（READ/WRITE）被触发时，执行对应事件的调度逻辑，\
把事件对应的 fiber 或回调函数交给 scheduler 调度执行，同时更新内部状态。
void IOManager::FdContext::triggerEvent(EVENT event){
    MY_ASSERT(event_type & event);
    event_type = (EVENT)(event_type & ~event);//执行了该事件，就从关注的事件中删除它
    EventContext &event_ctx = getContext(event);
    if(event_ctx.fiber){
        event_ctx.scheduler->pushtask(event_ctx.fiber);
    }else if(event_ctx.cb){
       event_ctx.scheduler->pushtask(event_ctx.cb);
    }else{
        STREAM_LOG_ERROR(g_logger)<<"fd_ctx 没有执行协程或回调";
    }
    event_ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(int threads,bool use_main,const std::string &name)
    :Scheduler(threads ,use_main ,name){
        //创建epoll句柄
        m_epfd = epoll_create1(EPOLL_CLOEXEC);
        MY_ASSERT2(m_epfd > 0,"epoll_create1 error");
        //创建eventfd句柄，用于线程事件通知
        m_eventfd_notify = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);//非阻塞+进程fork继承关闭 
        MY_ASSERT2(m_eventfd_notify > 0,"eventfd error");

        epoll_event ev;//关注的事件
        memset(&ev,0,sizeof(ev));
        ev.events = EPOLLIN | EPOLLET;//关注读事件(且是边缘触发(搭配的eventfd的非阻塞))
        ev.data.fd = m_eventfd_notify;
        
        int ret = epoll_ctl(m_epfd,EPOLL_CTL_ADD ,m_eventfd_notify,&ev);//将eventfd加入到epoll关注的事件中
        MY_ASSERT2( ret==0 ,"epoll_ctl error");
        resizeContext(64);
        // std::cout<<"start ---------------"<<std::endl;
        Start();
}
IOManager::~IOManager(){
    // std::cout<<"~iomanager"<<std::endl;
    Stop();
    close(m_epfd);
    close(m_eventfd_notify);
    for(size_t i = 0; i < m_fd_contexts.size() ; ++i){
        if(m_fd_contexts[i]){
            delete m_fd_contexts[i];
        }
    }
}

bool IOManager::addEvent(int fd ,EVENT event,std::function<void()> cb){
    FdContext *fd_ctx = nullptr;
    {
        RWMutex::WLock wlock(m_mutex);
        int size = m_fd_contexts.size();
        if(fd > size){
            resizeContext(fd * 1.5);
        }
        fd_ctx = m_fd_contexts[fd];
    }

    FdContext::MutexType::Lock lock(fd_ctx->mtx);
    //不能有重复的事件
    if(MY_UNLIKELY(fd_ctx->event_type & event)){
        STREAM_LOG_ERROR(g_logger)<<"same event"<<event;
        MY_ASSERT((fd_ctx->event_type & event));
        return false;
    }
    
    int opt = fd_ctx->event_type == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    epoll_event epev;
    epev.events = EPOLLET | fd_ctx->event_type | event;//边源触发
    epev.data.ptr = fd_ctx;
    int ret = epoll_ctl(m_epfd,opt,fd,&epev);
    if(ret != 0){
        STREAM_LOG_ERROR(g_logger)<<"epoll_ctl error";
        return false;
    }
    ++m_pendingCnt;
    fd_ctx->event_type = (EVENT)(event | fd_ctx->event_type);//更新事件
    auto& event_ctx = fd_ctx->getContext(event);
    MY_ASSERT(!event_ctx.cb && !event_ctx.fiber && !event_ctx.scheduler);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);        
    }else{
        event_ctx.fiber = Fiber::GetThis();
        MY_ASSERT2(event_ctx.fiber && event_ctx.fiber->GetState() == Fiber::RUNNING,
            "state = "<<event_ctx.fiber->GetState());
    }
    return true;
}

bool IOManager::delEvent(int fd ,EVENT event){
    {
        RWMutexType::RLock rlock(m_mutex);
        if(fd >= (int)m_fd_contexts.size()){
            return false;
        }
    }
    FdContext *fd_ctx;
    {
        RWMutexType::WLock wlock(m_mutex);
        fd_ctx = m_fd_contexts[fd];
    }
    FdContext::MutexType::Lock lock(fd_ctx->mtx);
    if(MY_UNLIKELY(!(fd_ctx->event_type & event))){
        STREAM_LOG_ERROR(g_logger)<<"没有此事件";
        MY_ASSERT(!(fd_ctx->event_type & event));
        return false;
    }
    
    EVENT new_events = (EVENT)(fd_ctx->event_type & ~event);
    int opt = new_events == NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_event epev;
    epev.events = EPOLLET | new_events;
    epev.data.ptr = fd_ctx;
    int ret = epoll_ctl(m_epfd,opt,fd,&epev);
    if( ret != 0){
        STREAM_LOG_ERROR(g_logger)<<"epoll_ctl error";
        return false;
    }
    --m_pendingCnt;
    fd_ctx->event_type = new_events;
    //将该事件重置
    auto& temp_context = fd_ctx->getContext(event);
    fd_ctx->resetContext(temp_context);
    return true;
}

//通过取消该事件来触发所对应事件的回调
bool IOManager::cancelEvent(int fd ,EVENT event){
    {
        RWMutexType::RLock rlock(m_mutex);
        if(fd >= (int)m_fd_contexts.size()){
            return false;
        }
    }

    FdContext *fd_ctx;
    {
        RWMutexType::WLock wlock(m_mutex);
        fd_ctx = m_fd_contexts[fd];
    }

    FdContext::MutexType::Lock lock(fd_ctx->mtx);
    if(MY_UNLIKELY(!(fd_ctx->event_type & event))){
        STREAM_LOG_ERROR(g_logger)<<"没有此事件";
        MY_ASSERT(!(fd_ctx->event_type & event));
        return false;
    }
    
    EVENT new_events = (EVENT)(fd_ctx->event_type & ~event);
    int opt = new_events == NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_event epev;
    epev.events = EPOLLET | new_events;
    epev.data.ptr = fd_ctx;
    int ret = epoll_ctl(m_epfd,opt,fd,&epev);
    if(ret != 0){
        STREAM_LOG_ERROR(g_logger)<<"epoll_ctl error";
        return false;
    }
    fd_ctx->triggerEvent(event);
    --m_pendingCnt;
    return true;
}

//取消对应文件描述符上的所有事件,来触发该文件描述符上所有事件的回调
bool IOManager::cancelAll(int fd ){
    {
        RWMutexType::RLock rlock(m_mutex);
        if(fd >= (int)m_fd_contexts.size()){
            return false;
        }
    }

    FdContext *fd_ctx;
    {
        RWMutexType::WLock wlock(m_mutex);
        fd_ctx = m_fd_contexts[fd];
    }
    
    FdContext::MutexType::Lock lock(fd_ctx->mtx);
    if(MY_UNLIKELY(!(fd_ctx->event_type))){
        STREAM_LOG_ERROR(g_logger)<<"没有任何事件";
        MY_ASSERT(!(fd_ctx->event_type));
        return false;
    }
    epoll_event epev;
    epev.events = 0;
    epev.data.ptr = fd_ctx;
    int ret = epoll_ctl(m_epfd,EPOLL_CTL_DEL,fd,&epev);
    if( ret != 0){
        STREAM_LOG_ERROR(g_logger)<<"epoll ctrl error";
        return false;
    }
    if(fd_ctx->event_type & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingCnt;
    }
    if(fd_ctx->event_type & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingCnt;
    }
    MY_ASSERT(fd_ctx->event_type == NONE);
    return true;
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

//通知协程，调度器有任务了
void IOManager::Notify(){
    if(!hasIdleThreads()){
        return ;
    }
    uint64_t val = 1;
    //写入eventfd计数+1从而触发通知机制
    int ret = write(m_eventfd_notify,&val,sizeof(val));
    if(ret == -1){
        STREAM_LOG_ERROR(g_logger)<<"write event error";
    }
    MY_ASSERT(ret > 0);
}
//返回是否可以停止
bool IOManager::CanStop(){
    // std::cout<<"scheduler canstop "<<Scheduler::CanStop()<<std::endl;
    // std::cout<<"pending cnt="<<m_pendingCnt<<std::endl;
    return  m_pendingCnt == 0 && !hasTimer() &&Scheduler::CanStop();
}
// 协程无任务可调度时执行idle协程,进行epoll_wait等待事件准备就绪
void IOManager::Idle(){
    STREAM_LOG_DEBUG(g_logger)<<"IO idle";
    const uint64_t MAXEVENTS = 256;
    epoll_event *epptr = new epoll_event[MAXEVENTS]();//在堆上开辟
    std::shared_ptr<epoll_event> epevs(epptr,[](epoll_event *ptr){
                                           delete[] ptr;
                                       });
    while(true){
        uint64_t next_timeout = 0;
        if(CanStop()){
            STREAM_LOG_DEBUG(g_logger)<<" exit idle ";
            break;
        }
        int cnt = 0;//有多少事件准备好了等待被执行
        //阻塞在epoll_wait里等待
        do{
            next_timeout = getNextTimer();//获取下一个定时器的超时时间
            static const uint64_t timeout = 5000;//默认超时时间为5秒
            if(next_timeout != ~0ull){
                next_timeout = (int)next_timeout > timeout ? timeout : next_timeout;
            } else {
                next_timeout = timeout;
            }
            STREAM_LOG_DEBUG(g_logger)<<"io idle epoll_wait timeout="<<next_timeout;
            cnt = epoll_wait(m_epfd,epptr,MAXEVENTS,(int)next_timeout);//等待next_timeout后若无时间，直接返回
            if(cnt < 0){
                STREAM_LOG_ERROR(g_logger)<<"epoll_wait error";
                continue;
            }

            if(cnt || next_timeout == 0){    
                break;
            }
            std::cout<<"io idle epoll_wait --- "<<std::endl;
            // sleep(1);
        }while(true);

        //获取所有过期的定时器回调
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()){
            pushtasks(cbs.begin(),cbs.end());//将所有过期的定时器回调添加到任务队列中
            cbs.clear();
        }

        //有事件准备好了
        for(size_t i = 0;i<cnt;i++){
            //若为事件触发句柄
            if(epptr[i].data.fd == m_eventfd_notify){
                uint64_t readbuf;
                // 非阻塞 + ET 模式下，这里读一次就够了；多次读也只是第二次返回 -1/EAGAIN
                while(read(m_eventfd_notify,&readbuf,sizeof(readbuf))>0);//这里其实可以不用循环读，因为eventfd没有设置为信号量模式,读一次就会完全清空里面的计数
                                                                       //之所以while是防止有用信号量模式，或者改用管道，因为事件都设置为了边缘触发模式，所以要循环读，防止一次性没有读完整
                continue;
            }
            
            FdContext *fd_ctx = (FdContext *)epptr[i].data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mtx);
            if(epptr[i].events & (EPOLLERR | EPOLLHUP)){
                epptr[i].events |= (EPOLLERR | EPOLLHUP) & fd_ctx->event_type;
            }

            //判断epoll返回的事件是否是用户所关注的事件
            int realevent = NONE;
            if(epptr[i].events & EPOLLIN){
                realevent |= READ;
            }
            if(epptr[i].events & EPOLLOUT){
                realevent |= WRITE;
            }
            if((fd_ctx->event_type & realevent) == NONE){//避免错误触发用户没有注册的事件
                continue;
            }
            
            //将剩下没有触发的事件重新添加到epoll中
            int leftevent = fd_ctx->event_type & (~realevent);
            int opt = leftevent == NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;//如果没有剩下的事件了，就删除，否则修改
            epptr[i].events = leftevent | EPOLLET;//关注的事件类型变为剩下没有触发的事件
            int ret = epoll_ctl(m_epfd,opt,fd_ctx->fd,&epptr[i]);
            if(ret != 0){
                STREAM_LOG_ERROR(g_logger)<<"epoll ctl error";
                continue;
            }

            //触发所关注的事件
            if(realevent & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingCnt;
            }
            if(realevent & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingCnt;
            }
        }
        // sleep(1);
        break;
    }
}
// 重置socket句柄上下文的容器大小
void IOManager::resizeContext(int n){
    m_fd_contexts.resize(n);
    for(size_t i = 0;i<m_fd_contexts.size();++i){
        if(!m_fd_contexts[i]){
            m_fd_contexts[i] = new FdContext;
            m_fd_contexts[i]->fd = i;
        }
    }
}
void IOManager::Haslatest(){
    Notify();
}

}
#endif
