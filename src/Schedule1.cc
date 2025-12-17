#include "Schedule1.h"
#include "Log.h"
#include "Macro.h"
#include "Util.h"   
#include "Hook.h"
namespace DYX{

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");
static thread_local Scheduler *t_scheduler = nullptr;   //线程内部的协程调度器  

static thread_local Fiber::Ptr t_root_schedule_fiber = nullptr;//根协程的调度协程

Scheduler::Scheduler(int threadnum,bool usemain ,const std::string &name )
    :use_main(usemain)
    ,m_name(name){
    if(usemain) threadnum--;//主线程也参与调度，则需要生成的调度线程数量减一
    MY_ASSERT(threadnum >= 0);
    m_buildThdCnt = threadnum;//记录需要生成的调度线程数量
    m_rootThreadId = DYX::GetThreadId();//获取主线程id
    Thread::setCurName(m_name);//将线程名字设置为调度器名字，便于调试
    t_scheduler = this;//将当前调度器赋值给线程内部的调度器指针

    if(use_main){
        m_rootFiber = Fiber::BuildMainFiber();//主协程无需开空间
        t_root_fiber = m_rootFiber.get();
        Fiber::SetCurFiber(t_root_fiber);//将主协程赋值给线程内部的协程指针
        //主线程中的调度协程
        t_root_schedule_fiber = std::make_shared<Fiber>(use_main,std::bind(&Scheduler::Run,this));
        Fiber::SetScheduleFiber(t_root_schedule_fiber.get());//主线程中的调度协程赋值给线程内部的调度协程指针
        MY_ASSERT2(t_schedule_fiber == t_root_schedule_fiber.get(),"t_schedule_fiber != t_root_schedule_fiber");//主线程中的调度协程赋值给线程内部的调度协程指针后，检查是否赋值成功
    }

}
void Scheduler::Start(){
    if(m_isRunning)//已经启动
        return;
    m_isRunning = true;//设置为运行中
    MY_ASSERT(m_threads.empty());
    m_threads.resize(m_buildThdCnt);//调整线程池大小
    //创建线程池
    for(int i = 0;i<m_buildThdCnt;i++){
        m_threads[i] = (std::make_shared<Thread>(std::bind(&Scheduler::Run,this),m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->Gettid());//将线程id加入线程id池
    }

}

void Scheduler::Run(){
    DYX::SetHookEnable(true);//每个线程开启钩子函数

    STREAM_LOG_DEBUG(g_logger)<<DYX::Thread::getCurName()<<" run ";
    //主线程在构造时判断了usemain,若为true,则主线程的调度协程在外部创立
    //子线程需要在内部构造调度协程
    if( DYX::GetThreadId() != m_rootThreadId ){//线程池里的子线程
        MY_ASSERT(Scheduler::GetThis() == nullptr);//默认子线程开始没有绑定调度器
        t_scheduler = this;//每个线程绑定自己的调度器
        t_schedule_fiber_ptr = Fiber::BuildScheduleFiber();//每个线程绑定自己的调度协程
        Fiber::SetScheduleFiber(t_schedule_fiber_ptr.get());//每个线程绑定自己的调度协程

        MY_ASSERT2(t_schedule_fiber == t_schedule_fiber_ptr.get(),"t_schedule_fiber != t_schedule_fiber_ptr");//每个线程绑定自己的调度协程后，检查是否赋值成功
    }

    //准备两个工具协程
    //1.空闲时跑 idle() 的协程，保证在“没有任务、但调度器还没停”的这段时间，有一个专门的 fiber 帮你处理“空转逻辑”和“停机逻辑”

    STREAM_LOG_DEBUG(g_logger)<<"idle fiber construct";
    Fiber::Ptr idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::Idle,this));
    //2.一个可复用的“执行回调”的协程壳（承载 std::function<void()>）
    Fiber::Ptr cb_fiber = nullptr;
    // std::shared_ptr<Task> task;//任务结构体,用于抽象协程和函数
    while(true){
        Task task;
        // task.Reset();//重置任务结构体
        bool need_notify = false; //唤醒其他线程来取任务的标准位
        //开始去任务队列取任务(临界区)
        {
            MutexType::WLock lock(m_mutex);
            auto it = m_tasks.begin();
            while(it != m_tasks.end()){
                if(it->cb || it->fiberptr){
                    //指定了任务在特定的线程里运作，但是该线程不是那个特定线程，则直接跳过
                    if(it->thread_id != -1 && it->thread_id != DYX::GetThreadId()){
                        ++it;
                        need_notify = true;
                        continue;
                    }
                    //取到了在本线程中运行的协程任务
                    task = std::move(*it);//将任务移动到task中(避免拷贝)性能更高
                    m_tasks.erase(it++);
                    it != m_tasks.end() ? need_notify |= true : need_notify |= false;//还有任务没有处理需要通知（没有扫描到最后）
                    break;
                }else{
                    m_tasks.erase(it++);
                }
            }
        }
        if(need_notify){//需要通知其他线程来取任务
            Notify();
        }
        
        //没有任务，进入idle协程(等待任务，或者等待停止通知)
        if(task.fiberptr == nullptr && task.cb == nullptr){//任务为空
            m_idleThdCnt++;//空闲线程数加一
            STREAM_LOG_DEBUG(g_logger)<<"idle fiber swap in";
            idle_fiber->SwapIn();//切换到idle协程
            m_idleThdCnt--;//空闲线程数减一
            if(CanStop()){//可以停止
                break;
            }
            idle_fiber->Reset(std::bind(&Scheduler::Idle,this));//重置idle协程函数
            continue;//从idle协程切换到调度协程,继续重新取任务
        }

        // 有任务，执行任务
        if(task.cb){
            if(cb_fiber){
                cb_fiber->Reset(task.cb);//重置协程函数
            }else{
                STREAM_LOG_DEBUG(g_logger)<<"cb fiber construct";
                cb_fiber = std::make_shared<Fiber>(task.cb);
            }

            STREAM_LOG_DEBUG(g_logger)<<"task cb fiber swap in";
            ++m_activeThdCnt;//活动线程数加一
            cb_fiber->SwapIn();//切换到cb协程
            --m_activeThdCnt;//活动线程数减一

            if(cb_fiber->GetState() == Fiber::TERM){//协程执行完毕
                cb_fiber->Reset(nullptr);//重置协程函数（短任务复用同一个协程栈）
            }else if(cb_fiber->GetState() == Fiber::HOLD){
                cb_fiber.reset();//不管理hold状态的协程,下次重新创建，避免共享栈问题
            }
        }else if(task.fiberptr ){
            if(task.fiberptr->GetState() == Fiber::TERM){
                task.fiberptr.reset();
                continue;
            }
            STREAM_LOG_DEBUG(g_logger)<<"task fiberptr swap in";
            ++m_activeThdCnt;//活动线程数加一
            task.fiberptr->SwapIn();
            --m_activeThdCnt;//活动线程数减一
            //切回调度协程如果上次运行的协程没有结束（yieldtohold），则设置为hold状态
            if(task.fiberptr->GetState() != Fiber::TERM){
                task.fiberptr->SetState(Fiber::HOLD);//设置为hold状态
            }
        }
        
        // m_notify = false;//设置为不需要通知线程
        
        // // 判断是否要停止调度
        // //如果可以停止，且任务队列为空，且活动线程数为0，则可以停止
        // if(CanStop()){//可以停止
        //     break;
        // }else{
        //     continue;//从cb协程切换到调度协程，说明任务队列不为空，继续取任务
        // }
    }   
}
void Scheduler::Stop(){
    if(!m_isRunning){//没有运行
        STREAM_LOG_ERROR(g_logger)<<"Scheduler::Stop() error: not running";
        return;
    }
    MY_ASSERT(m_enable_stop == false);//使能停止
    m_isRunning = false;//设置为不运行
    m_enable_stop = true;//使能停止

    if(use_main && t_root_schedule_fiber){
        //stop必须在主线程里调用
        MY_ASSERT2(m_rootThreadId == DYX::GetThreadId(),"m_rootThreadId != DYX::GetThreadId()");
        t_root_schedule_fiber->RootScheduleSwapIn();//切换到根调度协程
    }

    //通知所有线程停止
    //回收所有线程
    std::vector<Thread::Ptr> threads;
    {
        MutexType::WLock lock(m_mutex);
        threads.swap(m_threads);
    }

    for(auto& i:threads){
        i->join();
    }
}

//-----测试用-------
void Scheduler::Idle(){
    STREAM_LOG_DEBUG(g_logger)<<" in idle ";
    sleep(1);
}
void Scheduler::Notify(){
    // m_notify = true;//设置为需要通知线程
    STREAM_LOG_DEBUG(g_logger)<<"notify";
}
//-----------------------
bool Scheduler::CanStop(){
    MutexType::RLock lock(m_mutex);
    return m_enable_stop && m_tasks.empty() && (m_activeThdCnt == 0);
}
Scheduler::~Scheduler(){
    MY_ASSERT2(t_scheduler == this,"t_scheduler != this");//检查是否是当前线程的调度器
    t_scheduler = nullptr;
}

Scheduler *Scheduler::GetThis(){
    return t_scheduler;
}

}