#pragma once
#include <ucontext.h>
#include <functional>
#include <memory>


namespace DYX{

class Fiber : public  std::enable_shared_from_this<Fiber>{
private:
    Fiber();
public:
    enum State{
        INIT = 0,//初始状态
        RUNNING = 1,//运行状态
        TERM = 2,//结束状态
        HOLD = 3,//暂停状态
    };
    using Ptr = std::shared_ptr<Fiber>;
    Fiber(bool use_main,std::function<void()> sc_cb,int size = 1024 * 128);//默认调度协程(调度协程不需要开空间)
    Fiber(std::function<void()> cb,int size = 1024 * 128);
    ~Fiber();

    //协程切换的接口
    void SwapIn();
    //主线程中的调度协程切入接口
    void RootScheduleSwapIn();
    //重置协程函数
    void Reset(std::function<void()>cb);
    void SetState(State state);
    const Fiber::State& GetState() const;

    //创建主协程来调度
    static Fiber::Ptr BuildMainFiber();
    static Fiber::Ptr BuildScheduleFiber();
    static void SetCurFiber(Fiber* fiber);
    static void SetScheduleFiber(Fiber* fiber);

    //协程的入口函数(协程的上下文)
    static void ContextFunc();
    //主线程中的调度协程上下文函数
    static void RootScheduleContextFunc();
    static int GetId();
    static int TotalFibers();
    //协程切换到后台，并且设置为Hold状态
    static void YieldToHold();

    //获取当前协程的智能指针
    static Fiber::Ptr GetThis();
private:
    State m_state = INIT;//协程状态
    ucontext_t m_ctx;//协程的上下文
    void *m_stack_ptr = nullptr;//协程栈指针
    size_t m_stack_size = 0;//协程栈大小
    int m_fib_id = -1; //协程id
    std::function<void()> m_cb = nullptr;//协程要执行的任务函数，(由外界传入)
};

extern thread_local Fiber *t_curfiber;//当前在cpu上运行的协程
extern thread_local Fiber *t_schedule_fiber;//每一个线程的调度协程
extern thread_local Fiber::Ptr t_schedule_fiber_ptr;//每一个线程的调度协程智能指针

extern thread_local Fiber *t_root_fiber;//根协程（usemain = true时）


}
