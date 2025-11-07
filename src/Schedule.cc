#include "Schedule.h"
#include "Log.h"
#include "Macro.h"

/**

t_scheduler_fiber（调度返回点）
在工作线程里：t_scheduler_fiber 指向该线程的主协程（thread main fiber），也就是你说的“每个线程的调度协程”。
这是 run() 里 t_scheduler_fiber = Fiber::GetThis().get(); 做的事。
在根线程（use_caller=true 的那个）：t_scheduler_fiber 指向的是专门创建的 m_rootFiber（一个真正的 Fiber，用来跑 Scheduler::run()），
而不是该线程的主协程本身。——这一点和工作线程不同。

t_fiber（当前正在运行的协程）
它表示当前正在 CPU 上执行的协程，可能是任务协程、idle 协程、甚至是调度协程本身；不专指“任务协程”。切换到谁，t_fiber 就是谁。

t_threadFiber（线程主协程的智能指针）
是的，它持有线程主协程（每个线程第一次 Fiber::GetThis() 创建的那个“main fiber”）的 shared_ptr，
保证线程在退出前，该主协程不会被释放；同时也便于之后把控制流切回这个主协程。
 
主线程的t_schedule_fiber是m_rootfiber
子线程的t_schedule_fiber是自己的main_fiber(由t_threadFiber智能指针管理)

*/


namespace DYX{

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

static thread_local Fiber *t_schedule_fiber = nullptr;  //每个线程都要有一个调度协程
static thread_local Scheduler *t_scheduler = nullptr;   //线程内部的协程调度器

Scheduler::Scheduler(int threads ,bool use_caller ,const std::string &name)
    :m_name(name)
{
    MY_ASSERT(threads > 0);
    DYX::Fiber::GetThis();//创建主协程

    if(use_caller == true){//主线程也参与调度
        threads--;//主线程参与调度，则需要生成的调度线程数量减一
        MY_ASSERT(Scheduler::GetThis() == nullptr);//断定当前线程没有绑定调度器
        t_scheduler = this;//设置当前线程的调度器
        STREAM_LOG_DEBUG(g_logger)<<"m_rootfiber construct";
        m_rootfiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));//根协程变为调度协程,后面的use_caller标志位置为true
        t_schedule_fiber = m_rootfiber.get();

        DYX::Thread::setCurName(m_name);//将线程名字设置为调度器名字，便于调试

        m_rootThreadId = DYX::GetThreadId();//这里是获取内核级别的线程id
        m_threadIds.push_back(m_rootThreadId);//加入到线程id队列里，便于调度
    }else{
        m_rootThreadId = -1;//主线程不参与调度
    }
    m_need_built_thread_num = threads;//要新建的工作线程数
}
Scheduler::~Scheduler(){
    MY_ASSERT(m_stopping);
    t_scheduler = nullptr;
}


// 返回当前协程调度器
Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}

//返回当前线程的调度协程（主协程）
Fiber* Scheduler::GetMainFiber(){
    return t_schedule_fiber;
}

//启动协程调度器
void Scheduler::start(){
    MutexType::Lock lock(m_mutex);
    if(!m_stopping){
        return;
    }
    m_stopping = false;
    MY_ASSERT(m_threads.empty());//断言线程池为空
    m_threads.resize(m_need_built_thread_num);
    for(int i = 0;i<m_need_built_thread_num;++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),
                            m_name + "_" + std::to_string(i)));
        //将每个工作线程加入m_threadIds队列
        m_threadIds.push_back(m_threads[i]->Gettid());
    }
}


// 停止协程调度器
void Scheduler::stop(){
    m_autoStop = true;
    // 没有 worker 线程需要唤醒/回收（m_need_built_thread_num==0），也就无需后面的 Notify() 和 join()。
    // rootFiber 已是 INIT/TERM，说明不需要再切进去做清理；
    // 若队列也空、无活跃任务，那么 stopping() 为真，调度器事实上已经停止，stop() 可以立即返回
    //m_rootfiber存在，间接判断这是在主线程中
    if(m_rootfiber && m_need_built_thread_num == 0 &&
        (m_rootfiber->getState() == Fiber::TERM 
        || m_rootfiber->getState() == Fiber::INIT)){
            if(CanStop()){
                return;
            }
    }
    // 如果该早退条件不完全满足（例如还有排队任务或活跃计数没归零），
    // 就不会 return，流程会继续往下执行“正常停机流程”：广播 Notify() 唤醒、必要时 m_rootFiber->call() 再跑一轮收尾、最后回收线程

    if(m_rootThreadId != -1){
        MY_ASSERT(this == Scheduler::GetThis());
    }else{
        MY_ASSERT(this != Scheduler::GetThis());
    }

    m_stopping = true;//标记进入“停止阶段”，后续 stopping() 判断会用到它（配合 m_autoStop、队列是否为空、活跃数为 0 等）

    //如果主线程也参与调度（caller 模式有 m_rootFiber），也单独再“戳一下”主线程，确保主线程的调度循环也被唤醒去观察停止条件。
    if(m_rootfiber) Notify();
    
    //给所有 worker 线程发“有活儿/醒一醒”的信号。很多线程可能正停在 idle_fiber->swapIn()，
    //不唤醒它们就不会检查到 m_stopping 进而退出 run() 循环。
    for(int i = 0;i<m_need_built_thread_num;i++){
        Notify();
    }

    //回收所有线程
    std::vector<Thread::Ptr> threads;
    {
        MutexType::Lock lock(m_mutex);
        threads.swap(m_threads);
    }

    for(auto& i:threads){
        i->join();
    }

}

//协程调度函数(线程池里面每个线程的入口函数)
void Scheduler::run(){

    STREAM_LOG_DEBUG(g_logger)<<DYX::Thread::getCurName()<<" run ";
    if( DYX::GetThreadId() != m_rootThreadId ){//线程池里的子线程
        MY_ASSERT(Scheduler::GetThis() == nullptr);//默认子线程开始没有绑定调度器
        t_scheduler = this;//每个线程绑定自己的调度器
        STREAM_LOG_DEBUG(g_logger)<<"----sub thread will build main fiber to schedule----";
        t_schedule_fiber = Fiber::GetThis().get();//每个线程第一个创建的主协程作为它的调度协程
    }

    //准备两个工具协程
    //1.空闲时跑 idle() 的协程（让出 CPU、等待新任务）
    STREAM_LOG_DEBUG(g_logger)<<"idle_fiber construct";
    Fiber::Ptr idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle,this));
    //2.一个可复用的“执行回调”的协程壳（承载 std::function<void()>）
    Fiber::Ptr cb_fiber;
    
    //线程一直死循环，直到满足条件才退出
    while(true){
        Fiber_Func_Thread task;
        bool need_notify = false; //唤醒其他线程来取任务的标准位
        bool is_active = false;   //本线程是否取到任务的标志位
        //开始去任务队列取任务
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_task.begin();
            while(it != m_task.end()){

                //指定了任务在特定的线程里运作，但是该线程不是那个特定线程，则直接跳过
                if(it->thread_id != -1 && it->thread_id != DYX::GetThreadId()){
                    ++it;
                    need_notify = true;
                    continue;
                }
                //代码的健壮性检查
                MY_ASSERT(it->cb || it->fiber);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC){
                    ++it;
                    continue;
                }

                //取到了在本线程中运行的协程任务
                task = *it;
                m_task.erase(it++);
                m_activeThreadCount++;
                is_active = true;       //置标志位为活跃
                break;
            }
            need_notify |= (it != m_task.end());//还有任务没有处理需要通知（没有扫描到最后）
            // need_notify |= (!m_task.empty());这样写也可以，但是上面写法更精准，上面的写法更节省 唤醒、减少惊群
        }
        if(need_notify){//若需要唤醒其他线程
            Notify();
        }
        if(task.fiber && 
            task.fiber->getState() != Fiber::EXCEPT &&
            task.fiber->getState() != Fiber::TERM){
            task.fiber->swapIn();
            --m_activeThreadCount;
        
            if(task.fiber->getState() == Fiber::READY){//若状态为READY则重新加入任务队列(YeildtoReady的情况)
                pushtask(task.fiber);
            
            }else if(task.fiber->getState() != Fiber::EXCEPT &&
                    task.fiber->getState() != Fiber::TERM){//不为终止也没有异常，则挂起Hold
                task.fiber->m_state =  Fiber::HOLD;
            }
            task.reSet();
        }else if(task.cb){
            
            if(cb_fiber){//不是第一次执行任务了，说明之前cb_fiber初始化过，这里只需要复用
                cb_fiber->reSet(task.cb);//复用
            }else{//第一次执行cb任务
                STREAM_LOG_DEBUG(g_logger)<<"cb_fiber construct";
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reSet();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            STREAM_LOG_DEBUG(g_logger)<<"cb_fiber state = "<<cb_fiber->getState();
            if(cb_fiber->getState() == Fiber::READY){
                pushtask(cb_fiber);
            }else if( cb_fiber->getState() == Fiber::EXCEPT 
                        || cb_fiber->getState() == Fiber::TERM ){//cb任务执行结束，重置cb协程
                cb_fiber->reSet(nullptr);
            }else{
                cb_fiber->m_state = Fiber::HOLD;
            }
            task.reSet();
        }else{
            //健壮性与防御性处理，确保已结束/异常的协程不会被重复调度
            //这段逻辑是纯粹的健壮性与计数回滚，确保拿到但不可执行的任务不会导致活跃计数紊乱或线程错误进入 idle。
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            //如果空闲协程已经结束，说明调度器准备收尾（一般是在 stop() 后，idle() 会自己退出）。这时跳出 run() 循环，线程结束调度。
            if(idle_fiber->getState() == Fiber::TERM){
                STREAM_LOG_DEBUG(g_logger)<<"idle_fiber TERM";
                break;
            }

            //既没有fiber也没有cb
            //空闲等待
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;

            //  从 idle 返回但没有结束时，把它标记为 HOLD，表示“先挂起、下次还会继续作为 idle 使用”。
            // 这只是调度状态，不影响释放与否；不过这里 idle_fiber 有本地持有（shared_ptr 在变量里），因此不会被销毁，下次还会被 swapIn() 作为 idle。
            // 在这里手动把状态设为 HOLD，是为了将 idle 协程规范为“等待再次进入”的状态；当下一轮仍无任务时，调度器可以直接再次 swapIn() 这个 idle 协程
            if(idle_fiber->getState() != Fiber::TERM 
                &&idle_fiber->getState() != Fiber::EXCEPT){
                    idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }

}
//通知协程，调度器有任务了
void Scheduler::Notify(){
    STREAM_LOG_INFO(g_logger)<<"Notify";
}

//返回是否可以停止（光队列空和活跃线程数为0，是不可以停止的，\
因为还需要外部调用stop函数，在stop函数里置stopping和autostop状态）
bool Scheduler::CanStop(){
    MutexType::Lock lock(m_mutex);
    return m_stopping && m_autoStop 
        && m_task.empty() && m_activeThreadCount == 0;
}
// 协程无任务可调度时执行idle协程
void Scheduler::idle(){
    STREAM_LOG_INFO(g_logger)<<"in the idle func";
    while(!CanStop()){//忙等
        Fiber::YieldToHold();
    }
}


}
