#include "Fiber.h"
#include "Macro.h"
#include "Log.h"
#include "Config.h"
#include "Schedule.h"
#include <atomic>

namespace DYX {

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

static thread_local Fiber* t_fiber = nullptr;           //切换的协程（用裸指针切换更迅速）专用于“此刻正在跑谁”
static thread_local Fiber::Ptr t_threadFiber = nullptr; //这里保存的每个线程的主协程 长期持有，保证“回得去且不被释放”

static  std::atomic<uint64_t> s_fiber_id {0};   //原子变量，记录协程id
static  std::atomic<uint64_t> s_fiber_count {0};//原子变量，记录协程总数

//配置变量（协程的栈大小）
static DYX::ConfigVar<uint32_t>::Ptr g_stack_size\
                        = DYX::ConfigManager::Lookup("fiber_stack_size",(uint32_t)(1024 * 128) ,"协程栈的大小");

//协程内存空间分配器(开辟的堆空间，用户自己管理)
class Allocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp) {
        return free(vp);
    }
};

//主协程
Fiber::Fiber()
    :m_id(++s_fiber_id){
    SetThis(this);
    m_state = EXEC;//状态设置为执行中状态
    if(getcontext(&m_ctx)){
        MY_ASSERT2(false,"getcontext");
    }
    ++s_fiber_count;
    STREAM_LOG_DEBUG(g_logger)<<"main fiber construct";
}

//子协程
//用于创建真正的协程任务对象
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)//use_caller 是否在MainFiber上调度
    :m_cb(cb)
    ,m_id(++s_fiber_id)
{
    s_fiber_count++;
    m_stack_size = stacksize == 0 ? g_stack_size->getValue() : stacksize;
    m_stack_ptr = Allocator::Alloc(m_stack_size);

    if(getcontext(&m_ctx)){
        MY_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link = nullptr;//uc_link 为空，代表该协程执行完后不会自动跳到其他上下文
    // 绑定协程专用栈（ss_sp 指向栈底，ss_size 指定大小）。
    m_ctx.uc_stack.ss_sp = m_stack_ptr;
    m_ctx.uc_stack.ss_size = m_stack_size;

    if(use_caller){//在mainfiber上调度
        makecontext(&m_ctx, Fiber::CallerMainFunc,0);
    }else{
        makecontext(&m_ctx, Fiber::MainFunc,0);
    }
    STREAM_LOG_DEBUG(g_logger)<<"sub_fiber id : "<<m_id;

} 
Fiber::~Fiber(){
    s_fiber_count--;
    if(m_stack_ptr){//子协程（因为只要子协程才开辟空间）
        MY_ASSERT( m_state == TERM || m_state == INIT || m_state == EXCEPT);
        Allocator::Dealloc(m_stack_ptr);
    }else{//默认主协程没有开辟空间
        MY_ASSERT(!m_cb && m_state == EXEC);//主协程没有回调,且状态一直是执行

        Fiber* cur = t_fiber;//获取当前线程的协程指针
        if(cur == this) {//若与主协程相同，则处于主协程中
            SetThis(nullptr);
        }   
    }

    STREAM_LOG_DEBUG(g_logger)<<"~Fiber id : "<<m_id << " , total num : "<<s_fiber_count;
}

//协程执行函数重置，复用同一个协程对象，即对一个已经开辟了的空间重新复用
void Fiber::reSet(std::function<void()> cb){
    MY_ASSERT(m_stack_ptr);
    MY_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = cb;
    if(getcontext(&m_ctx)){
        MY_ASSERT2(false,"getcontext");
    }

    //因为 getcontext(&m_ctx) 会重新初始化整个 ucontext_t 结构体，
    // 导致原来设置的栈指针和大小都被覆盖掉。
    // 所以 reset 时必须重新指定栈空间。
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack_ptr;
    m_ctx.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ctx,Fiber::MainFunc,0);
    m_state = INIT;
}

//将当前协程切换到运行状态
void Fiber::swapIn(){
    SetThis(this);              //设置当前的运行子协程
    MY_ASSERT(m_state != EXEC); //切换进来的子协程初始状态不能是运行状态
    m_state = EXEC;
    if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx),&m_ctx)){//与每个线程的调度协程（主协程）进行上下文切换
        MY_ASSERT2(false , "swapcontext");
    }
    
}

//将当前协程切换到后台
void Fiber::swapOut(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx,&(Scheduler::GetMainFiber()->m_ctx))){//与每个线程的调度协程（主协程）进行上下文切换
        MY_ASSERT2(false , "swapcontext");
    }
}


//当use_caller = true时，调度协程是scheduler的m_rootFibe，\
这里是用于让main_fiber切入m_rootFiber进行协程调度
void Fiber::call(){
    SetThis(this);
    MY_ASSERT(m_state != Fiber::EXEC);
    m_state = Fiber::EXEC;
    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){//在caller线程中，m_rootFiber与主协程切换上下文
        MY_ASSERT2(false,"swapcontext");
    }
}

//同理，这是m_rootFiber调度结束返回主协程
void Fiber::back(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){//m_rootFiber处理完所有任务后back返回到主协程的上下文
        MY_ASSERT2(false , "swapcontext");
    }
}


/*------------------静态成员函数----------------------*/



//返回当前协程（智能指针管理）
// “当前线程第一次使用协程系统”时创建主协程
Fiber::Ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::Ptr main_fiber (new DYX::Fiber());//无参构造主协程
    MY_ASSERT(t_fiber == main_fiber.get());//fiber无参构造的时候，默认setthis设置了t_fiber，这里断言一下
    t_threadFiber = main_fiber;
    return t_threadFiber;
}
//设置当前协程
void Fiber::SetThis(Fiber* f){
    t_fiber = f;
}

//将当前协程切换到后台,并设置为READY状态
void Fiber::YieldToReady(){
    Fiber *cur = t_fiber;
    MY_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

// 将当前协程切换到后台,并设置为HOLD状态
void Fiber::YieldToHold(){
    Fiber *cur = t_fiber;
    MY_ASSERT(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

//协程入口函数,执行完成返回到线程主协程
void Fiber::MainFunc(){
    Fiber *cur = t_fiber;//仅使用 t_fiber（裸指针）避免智能指针冻结导致引用计数泄露
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch(std::exception &ex){
        cur->m_state = EXCEPT;
        STREAM_LOG_ERROR(g_logger) << "Fiber Except :"<<ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << DYX::BacktraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        STREAM_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << DYX::BacktraceToString();
    }

    cur->swapOut();
    //（子协程上下文 m_ctx.uc_link = nullptr。这意味着协程函数（MainFunc）返回时没有“落脚点”。不 swapOut() 的话，协程执行完就到头了，线程直接结束；）
    // swapOut() 里面是 swapcontext(&m_ctx, &(t_threadFiber->m_ctx))：
    // 这会把当前协程的 CPU 寄存器 + 栈指针保存到 m_ctx，然后直接跳转到主协程 t_threadFiber->m_ctx 去执行.\
    控制流离开了 MainFunc()，但函数帧并没有按“返回”那样被销毁。
    // （ucontext 规定 uc_link==nullptr 时，context 函数返回就终止线程）
    // 因此不让 MainFunc() 走 return，而是显式 swapOut() 回主协程。
    
    /*-----说明-------
        若cur使用智能指针需要注意
        普通 return 会展开栈帧，执行局部变量析构；
        swapcontext 只是冻结当前栈帧并跳走，局部变量（比如你在 MainFunc() 里拿到的 Fiber::Ptr cur）不会析构。
    */

    MY_ASSERT2(false,"never reach");
    

}

//Caller协程的入口函数
void Fiber::CallerMainFunc(){
    Fiber *cur = t_fiber;//仅使用 t_fiber（裸指针）避免智能指针冻结导致引用计数泄露
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch(std::exception &ex){
        cur->m_state = EXCEPT;
        STREAM_LOG_ERROR(g_logger) << "Fiber Except :"<<ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << DYX::BacktraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        STREAM_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << DYX::BacktraceToString();
    }
    cur->back();
    MY_ASSERT2(false,"never reach");
}
//返回当前协程的总数量   
uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

//获取当前协程的id
uint64_t Fiber::GetFiberId(){
    if(t_fiber)
        return t_fiber->getId();
    return 0;
}


}
