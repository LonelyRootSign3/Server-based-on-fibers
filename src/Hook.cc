#include "Hook.h"
#include "Fiber1.h"
#include "IOManager.h"
#include "Macro.h"
#include "FdManager.h"
#include "Config.h"
#include <dlfcn.h>
static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");  

namespace DYX{
static DYX::ConfigVar<int>::Ptr g_tcp_connect_timeout =
    DYX::ConfigManager::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");//connect超时时间,从配置中读取

static thread_local bool t_hook_enable = false;

bool IsHookEnable(){
    return t_hook_enable;
}

void SetHookEnable(bool flag){
    t_hook_enable = flag;
}

#define HOOK_FUN(XX)\
        XX(sleep) \
        XX(usleep) \
        XX(nanosleep) \
        XX(socket) \
        XX(connect) \
        XX(accept) \
        XX(read) \
        XX(readv) \
        XX(recv) \
        XX(recvfrom) \
        XX(recvmsg) \
        XX(write) \
        XX(writev) \
        XX(send) \
        XX(sendto) \
        XX(sendmsg) \
        XX(close) \
        XX(fcntl) \
        XX(ioctl) \
        XX(getsockopt) \
        XX(setsockopt)

void hookinit(){
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
    //初始化宏
    #define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);//在你的动态库加载时（constructor）通过 dlsym(RTLD_NEXT, ...) 找到真正的系统函数
    // RTLD_NEXT 的意思：继续沿着动态链接链往下找 系统原始函数                                      
        HOOK_FUN(XX);
    #undef XX
    is_inited = true;
}
static uint64_t s_connect_timeout = -1;

struct HookIniter{
    HookIniter(){
        hookinit();
        s_connect_timeout = g_tcp_connect_timeout->getValue();//从配置中读取connect超时时间
                g_tcp_connect_timeout->addChangeCallback([](const int& old_value, const int& new_value){
                STREAM_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

// 全局静态对象，在程序启动时自动初始化（先于main）
static HookIniter _hook_initer;


}

struct timer_info{//定时器的信息
    int canceled = 0;
};


template<typename OriginFunc,typename... Args>
int do_io(int fd , OriginFunc func,const char *func_name,
          uint32_t event,int timeout_ms,Args&&... args){
    if(!DYX::IsHookEnable()){//没有启动钩子，直接调用系统函数
        return func(fd,std::forward<Args>(args)...);
    }
    DYX::FdCtx::Ptr fd_ctx = DYX::FdMgr::GetInstance()->get(fd);
    if(!fd_ctx){
        return func(fd,std::forward<Args>(args)...);
    }
    if(fd_ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    if(!fd_ctx->isSocket() || fd_ctx->isUserNonblock()){//非socket或用户设置为非阻塞
        return func(fd,std::forward<Args>(args)...);
    }
    uint64_t timeout = fd_ctx->getTimeout(timeout_ms);
    std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();//记录定时器是否要取消信息

retry:
    //先执行真实的系统调用
    size_t ret = func(fd,std::forward<Args>(args)...);
    while(ret == -1 && errno == EINTR){//被系统打断，需要重试
        ret = func(fd,std::forward<Args>(args)...);
    }
    if(ret == -1 && errno == EAGAIN){//数据没有来，需要等待，将协程挂起
        DYX::IOManager *iom = DYX::IOManager::GetThis();
        DYX::Timer::Ptr timer;//构建定时器
        std::weak_ptr<timer_info> wtinfo = tinfo;//条件定时器的条件(原定时器是否取消过)
        //如果设置了超时时间
        if(timeout != (uint64_t)-1){
            timer = iom->addConditionTimer(timeout, [iom,curfib,wtinfo](){
                auto timeinfo = wtinfo.lock();
                if(!timeinfo || timeinfo->canceled){//定时器被取消了，说明在这之前，该任务被执行过了，直接返回
                    return ;
                }
                timeinfo->canceled = ETIMEDOUT;
                iom->cancelEvent(fd, (DYX::IOManager::Event)(event));
            },wtinfo);
        }
        int ret = iom->addEvent(fd, (DYX::IOManager::Event)(event));//将该协程登记到fd的READ/WRITE事件上,如果触发事件，则唤醒协程
        if(MY_UNLIKELY(ret)){
            if(timer){
                timer->cancel();
            }
            return -1;
        }else{
            DYX::Fiber::YieldToHold();
            if(timer){//防止定时器再次被触发
                timer->cancel();
            }
            if(tinfo->canceled){//如果超时返回，则返回超时错误
                errno = tinfo->canceled;
                return -1;
            }
        }
        goto retry;
    }

    return ret;
}





extern "C" {

    //将头文件的声明先定义(用宏简化代码)
#define XX(name) name##_fun name##_f = nullptr;
        HOOK_FUN(XX);
#undef XX

unsigned int sleep (unsigned int seconds){
    if(!DYX::IsHookEnable()){
        return sleep_f(seconds);
    }
    DYX::Fiber::Ptr curfib = DYX::Fiber::GetThis();
    DYX::IOManager *iom = DYX::IOManager::GetThis();
    auto func = [iom,curfib](){
        iom->pushtask(curfib);
    };
    curfib.reset();//不再管理这个对象
    iom->addTimer(seconds*1000,func,false);   
    DYX::Fiber::YieldToHold();
    // STREAM_LOG_DEBUG(g_logger)<<"after yield";
    return 0;
}
int usleep(useconds_t usec) {
    if(!DYX::IsHookEnable()){
        return usleep_f(usec);
    }
    DYX::Fiber::Ptr curfib = DYX::Fiber::GetThis();
    DYX::IOManager *iom = DYX::IOManager::GetThis();
    auto func = [iom,curfib](){
        iom->pushtask(curfib);
    };
    curfib.reset();//不再管理这个对象
    iom->addTimer(usec/1000,func,false);   
    DYX::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!DYX::IsHookEnable()){
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    DYX::Fiber::Ptr curfib = DYX::Fiber::GetThis();
    DYX::IOManager *iom = DYX::IOManager::GetThis();
    auto func = [iom,curfib](){
        iom->pushtask(curfib);
    };
    curfib.reset();//不再管理这个对象
    iom->addTimer(timeout_ms,func,false);   
    DYX::Fiber::YieldToHold();
    return 0;
}

int socket(int domain,int type,int protocol){
    if(!DYX::IsHookEnable()){
        return socket_f(domain,type,protocol);
    }
    int fd = socket_f(domain,type,protocol);
    if(fd < 0){
        return fd;
    }
    DYX::FdMgr::GetInstance()->get(fd,true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, int timeout_ms) {
    if(!DYX::IsHookEnable()){
        return connect_f(fd,addr,addrlen);
    }
    auto ctx = DYX::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket() || ctx->isUserNonblock()){//非socket或用户设置为非阻塞
        return connect_f(fd,addr,addrlen);
    }
    int n = connect(fd,addr,addrlen);
    if(n == 0) return n;//成功直接返回
    else if( n!=-1 || errno != EINPROGRESS) return n;

    //错误，且错误码为EINPROGRESS，说明连接正在进行中，需要进行异步处理
    
    DYX::IOManager *iom = DYX::IOManager::GetThis();
    DYX::Timer::Ptr timer;
    std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();
    std::weak_ptr<timer_info> winfo = tinfo;
    if(timeout_ms != (uint64_t)-1){
        timer = iom->addConditionTimer(timeout_ms,[winfo,fd,iom](){
            auto timeinfo = winfo.lock();
            if(!timeinfo || timeinfo->canceled){//定时器被取消了，说明在这之前，该任务被执行过了，直接返回
                return ;
            }
            timeinfo->canceled = ETIMEDOUT;
            iom->cancelEvent(fd, (DYX::IOManager::WRITE));
        },winfo);
    }
    int rt = iom->addEvent(fd,DYX::IOManager::WRITE);
    if(rt == 0){
        DYX::Fiber::YieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->canceled){//如果超时返回，则返回超时错误
            errno = tinfo->canceled;
            return -1;
        }
    }else{
        if(timer){
            timer->cancel();
        }
        STREAM_LOG_ERROR(g_logger)<<"connect addEvent("<<fd<<", WRITE) error";
        return -1;
    }

    // epoll 可写 表示连接完成，但不表示成功
    // TCP 连接的失败信息（如拒绝、超时）不会通过 epoll 的事件返回
    // 要检查 connect 的真正结果必须读取内核中的 SO_ERROR
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {//error为0表示连接成功
        return 0;
    } else {
        errno = error;
        return -1;
    }

}

int connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(fd, addr, addrlen, DYX::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", DYX::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        DYX::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}


ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", DYX::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", DYX::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", DYX::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", DYX::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", DYX::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", DYX::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", DYX::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", DYX::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", DYX::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", DYX::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd){
    if(!DYX::IsHookEnable()){
        return close_f(fd);
    }
    DYX::FdCtx::Ptr ctx = DYX::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = DYX::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        DYX::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}



}
