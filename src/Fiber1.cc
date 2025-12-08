#include "Fiber1.h"
#include "Macro.h"
#include "Log.h"

namespace DYX{

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

thread_local Fiber *t_curfiber = nullptr;//当前在cpu上运行的协程
thread_local Fiber *t_schedule_fiber = nullptr;//每一个线程的调度协程
thread_local Fiber *t_root_fiber = nullptr;//根协程（usemain = true时）

thread_local Fiber::Ptr t_schedule_fiber_ptr = nullptr;//每一个线程的调度协程智能指针


static std::atomic<uint32_t> s_fiber_id{0};//协程id
static std::atomic<uint32_t> s_fiber_count{0};//协程总数量

static void *Alloc(size_t size){
    return malloc(size);
}
static void Dealloc(void *vp){
    free(vp);
}

void Fiber::SetCurFiber(Fiber* fiber){
    t_curfiber = fiber;
}

void Fiber::SetScheduleFiber(Fiber* fiber){
    t_schedule_fiber = fiber;
}

void Fiber::SetState(State state){
    m_state = state;
}

const Fiber::State& Fiber::GetState() const{
    return m_state;
}

Fiber::Fiber()//协程的落脚点，不需要开空间
    :m_fib_id(++s_fiber_id){
    s_fiber_count++;
    if(getcontext(&m_ctx)){
        MY_ASSERT2(false ,"getcontext");
    }
    STREAM_LOG_DEBUG(g_logger)<<"main or schedule fiber construct "<<m_fib_id;
}

//主线程参与协程调度后的额外参与调度的调度协程
Fiber::Fiber(bool use_main,std::function<void()> sc_cb,int size)
    :m_fib_id(++s_fiber_id){
        MY_ASSERT2(use_main == true,"use_main must be true");
        MY_ASSERT2(t_root_fiber != nullptr,"root fiber is nullptr");
        if(getcontext(&m_ctx)){
            MY_ASSERT2(false ,"getcontext");
        }
        m_cb = sc_cb;//调度协程需要执行的调度函数
        m_ctx.uc_link = &(t_root_fiber->m_ctx);
        MY_ASSERT2(m_ctx.uc_link != nullptr,"uc_link is nullptr");//根协程的上下文不能为nullptr
        m_stack_ptr = Alloc(size);
        m_stack_size = size;
        m_ctx.uc_stack.ss_sp = m_stack_ptr;
        m_ctx.uc_stack.ss_size = m_stack_size;

        makecontext(&m_ctx,RootScheduleContextFunc,0);
        s_fiber_count++;
        SetScheduleFiber(this);
        STREAM_LOG_DEBUG(g_logger)<<"root schedule_fiber construct "<<m_fib_id;
}

//普通子协程中创建的工作协程
Fiber::Fiber(std::function<void()> cb,int size)//执行任务的子协程（创建）
    :m_fib_id(++s_fiber_id),m_cb(cb)
{
    MY_ASSERT2(t_schedule_fiber != nullptr,"schedule fiber is nullptr");
    MY_ASSERT2(cb != nullptr,"cb is nullptr");
    if(getcontext(&m_ctx)){
        MY_ASSERT2(false ,"getcontext");
    };

    m_ctx.uc_link = &(t_schedule_fiber->m_ctx);//协程执行完后切回调度协程
    MY_ASSERT2(m_ctx.uc_link != nullptr,"uc_link is nullptr");//调度协程的上下文不能为nullptr

    m_stack_ptr = Alloc(size);
    m_stack_size = size;

    m_ctx.uc_stack.ss_sp = m_stack_ptr;
    m_ctx.uc_stack.ss_size = m_stack_size;
    // 因为 makecontext 是基于 ucontext_t 对象的，\
    且每个协程（每个 Fiber）都会有不同的 m_ctx，\
    因此不同 Fiber 的上下文是独立的，即使它们都调用了 Fiber::ContextFunc
    makecontext(&m_ctx,ContextFunc,0);
    s_fiber_count++;
    STREAM_LOG_DEBUG(g_logger)<<"woker_fiber or idle_fiber construct "<<m_fib_id;
}
Fiber::~Fiber(){
    s_fiber_count--;
    if(m_stack_ptr){
        Dealloc(m_stack_ptr);  
    }
    STREAM_LOG_DEBUG(g_logger)<<"~Fiber id : "<<m_fib_id<<" , total num : "<<s_fiber_count;
}

//创建主协程来调度
Fiber::Ptr Fiber::BuildMainFiber(){
    STREAM_LOG_DEBUG(g_logger)<<"main_fiber construct";
    return std::shared_ptr<Fiber>(new Fiber());
}
//创建调度协程
Fiber::Ptr Fiber::BuildScheduleFiber(){
    STREAM_LOG_DEBUG(g_logger)<<"schedule_fiber construct";
    return std::shared_ptr<Fiber>(new Fiber());
}
void Fiber::SwapIn(){
    SetCurFiber(this);
    m_state = RUNNING;//设置为运行状态
    if(swapcontext(&(t_schedule_fiber->m_ctx),&m_ctx)){
        MY_ASSERT2(false,"swapcontext");
    }
}

void Fiber::RootScheduleSwapIn(){
    SetCurFiber(this);
    m_state = RUNNING;//设置为运行状态
    if(swapcontext(&(t_root_fiber->m_ctx),&m_ctx)){
        MY_ASSERT2(false,"swapcontext");
    }
}

// //重置协程函数
// void Fiber::Reset(std::function<void()>cb){
//     MY_ASSERT(cb);
//     m_cb = cb;
// }
//重置协程函数
void Fiber::Reset(std::function<void()> cb) {
    MY_ASSERT(cb);
    MY_ASSERT(m_stack_ptr);         // 必须是有栈的 worker fiber
    m_cb = std::move(cb);
    m_state = INIT;

    if (getcontext(&m_ctx)) {
        MY_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = &(t_schedule_fiber->m_ctx);
    m_ctx.uc_stack.ss_sp   = m_stack_ptr;
    m_ctx.uc_stack.ss_size = m_stack_size;
    makecontext(&m_ctx, ContextFunc, 0);
}

//协程的入口函数(协程的上下文),静态成员函数
void Fiber::ContextFunc(){
    MY_ASSERT2(t_curfiber != nullptr,"t_curfiber is nullptr");
    STREAM_LOG_DEBUG(g_logger)<<"in the ContextFunc";
    if(t_curfiber->m_cb){
        t_curfiber->m_cb();
        t_curfiber->m_cb = nullptr;
    }else{
        STREAM_LOG_ERROR(g_logger)<<"fiber id : "<<t_curfiber->m_fib_id<<" , cb is nullptr";
    }
    //执行完任务后切回调度协程
    t_curfiber->m_state = TERM;//设置为结束状态
    SetCurFiber(t_schedule_fiber);//当前运行协程继续为调度协程
}

//主线程中的调度协程上下文函数
void Fiber::RootScheduleContextFunc(){
    MY_ASSERT2(t_schedule_fiber != nullptr,"t_scheduler is nullptr");
    if(t_schedule_fiber->m_cb){
        t_schedule_fiber->m_cb();
        t_schedule_fiber->m_cb = nullptr;
    }else{
        STREAM_LOG_ERROR(g_logger)<<"root schedule fiber id : "<<t_schedule_fiber->m_fib_id<<" , cb is nullptr";
    }
    //执行完任务后切回主线程
    t_schedule_fiber->m_state = TERM;//设置为结束状态
    SetCurFiber(t_root_fiber);//当前运行协程继续为主线程
}

void Fiber::YieldToHold(){
    MY_ASSERT2(t_curfiber != nullptr,"t_curfiber is nullptr");
    MY_ASSERT2(t_curfiber->m_state == RUNNING || t_curfiber->m_state == HOLD ,"t_curfiber state is not RUNNING or HOLD");
    t_curfiber->m_state = HOLD;//设置为Hold状态
    if(swapcontext(&(t_curfiber->m_ctx),&(t_schedule_fiber->m_ctx))){
        MY_ASSERT2(false,"swapcontext");
    }
}
//获取当前协程的智能指针
Fiber::Ptr Fiber::GetThis(){
    if(t_curfiber){
        return t_curfiber->shared_from_this();
    }
    return nullptr;
}

int Fiber::GetId(){
    if(t_curfiber){
        return t_curfiber->m_fib_id;
    }
    return -1;
}
int Fiber::TotalFibers(){
    return s_fiber_count;
}

}
