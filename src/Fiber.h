#pragma once
#include <ucontext.h>
#include <functional>
#include <memory>
namespace DYX{

class Scheduler;

class Fiber: public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
private:
    Fiber();//每个线程第一个协程的构造（主协程）

public:
    //协程状态
    enum State {
        INIT, /// 初始化状态
        HOLD,   /// 暂停状态
        EXEC,   /// 执行中状态
        TERM,   /// 结束状态
        READY,  /// 可执行状态
        EXCEPT  /// 异常状态
    };
    using Ptr = std::shared_ptr<Fiber>;
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);//use_caller 是否在MainFiber上调度
    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态(复用同一个协程对象)
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reSet(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    //当use_caller = true时，调度协程是scheduler的m_rootFibe，\
    这里是用于让main_fiber切入m_rootFiber进行协程调度
    void call();

    //同理，这是m_rootFiber调度结束返回主协程
    void back();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state;}
public:
    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 运行协程
     */
    static void SetThis(Fiber* f);
    
    //返回当前所在的协程
    static Fiber::Ptr GetThis();

    /**
     * @brief 将当前协程切换到后台,并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台,并设置为HOLD状态
     * @post getState() = HOLD
     */
    static void YieldToHold();

     /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
    static void MainFunc();        

        /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
    static void CallerMainFunc();

    static uint64_t TotalFibers();  //返回当前协程的总数量
    static uint64_t GetFiberId();   //获取当前协程的id
private:
    uint64_t m_id = 0;          /// 协程id
    uint32_t m_stack_size = 0;   /// 协程运行栈大小
    State m_state = INIT;       /// 协程状态
    ucontext_t m_ctx;           /// 协程上下文
    void* m_stack_ptr = nullptr;    /// 协程运行栈指针
    std::function<void()> m_cb = nullptr; /// 协程运行函数
};



}

