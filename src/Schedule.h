#pragma once

#include "Thread.h"
#include "Fiber.h"

namespace DYX{

class Scheduler{
public:
    using Ptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;
       /**
     * @brief 构造函数
     * @param[in] threads 参与调度的线程数量(包括主线程)
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(int threads ,bool use_caller ,const std::string &name);
    virtual ~Scheduler();
        /**
     * @brief 返回协程调度器名称
     */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 返回当前协程调度器
     */
    static Scheduler* GetThis();

    /**
     * @brief 返回当前线程的主协程（调度协程）
     */
    static Fiber* GetMainFiber();

    /**
     * @brief 启动协程调度器
     */
    void start();

    /**
     * @brief 停止协程调度器
     */
    void stop();
    
    //单个任务加入，传入协程或者函数
    template<class FuncOrFiber>
    void pushtask(FuncOrFiber &task,pid_t thread = -1){
        bool need_notify = false;//是否通知（如果任务队列为空，则通知加入任务协程，并进行调度）
        {
            MutexType::Lock lock(m_mutex);
            need_notify = pushtaskNolock(task,thread);
        }
        if(need_notify){
            Notify();
        }
    }

    //批量任务加入，传入的是协程或者函数如vector<Fiber::Ptr> 或者vector<function<void()>>
    template<class TasksIterator>
    void pushtasks(TasksIterator begin , TasksIterator end){
        bool need_notify = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin!=end){
                need_notify = need_notify || (pushtaskNolock(&(*begin),-1));//传入任务变量地址（清空源对象），这样在构造时避免了拷贝多余的对象
                begin++;
            }
        }
        if(need_notify){
            Notify();
        }
    }
private:
    //协程调度（任务插入）
    template<class FuncOrFiber>
    bool pushtaskNolock(FuncOrFiber &task,pid_t thread){
        bool need_notify = m_task.empty();
        Fiber_Func_Thread fct(task,thread);//用函数或者协程构造任务
        if(fct.cb || fct.fiber){
            m_task.push_back(fct);
        }
        return need_notify;
    }

private:
    //任务类型
    struct Fiber_Func_Thread{
        Fiber::Ptr fiber;//协程
        std::function<void()> cb;//协程执行函数
        pid_t thread_id; //线程id
        //无参构造
        Fiber_Func_Thread():thread_id(-1){}
        
        //协程智能指针构造（协程引用计数+1）
        Fiber_Func_Thread(Fiber::Ptr f,pid_t id)
            :thread_id(id),fiber(f){}
        
        //协程智能指针的指针构造（目的是控制引用计数（转移智能指针持有对象），确保内存释放安全）
        Fiber_Func_Thread(Fiber::Ptr *f,pid_t id)
            :thread_id(id) {
                fiber.swap(*f);//转移持有者，引用计数不变
            }

        //函数对象构造
        Fiber_Func_Thread(std::function<void()> c,pid_t id)
            :thread_id(id),cb(c){}    

        //函数对象指针构造    
        Fiber_Func_Thread(std::function<void()> *c,pid_t id)
            :thread_id(id){
                cb.swap(*c);
            } 
        //重置数据
        void reSet(){
            fiber = nullptr;
            cb = nullptr;
            thread_id = -1;
        }
        
    };
protected:
    //通知协程，调度器有任务了
    virtual void Notify();
    //协程调度函数
    void run();
    //返回是否可以停止
    virtual bool CanStop();
    // 协程无任务可调度时执行idle协程
    virtual void idle();
private:
    MutexType m_mutex;
    std::string m_name;//调度器的名字
    std::vector<Thread::Ptr> m_threads;//线程池
    std::list<Fiber_Func_Thread> m_task;//任务队列（任务可以是协程也可以是函数）
    Fiber::Ptr m_rootfiber; //根协程（use_caller为true时有效，作为调度协程）
protected:
    /// 协程下的线程id数组
    std::vector<pid_t> m_threadIds;
    /// 工作线程数量
    size_t m_need_built_thread_num = 0;
    /// 工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    /// 空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    /// 是否正在停止
    bool m_stopping = true;
    /// 是否自动停止（用于调度器的停止）
    bool m_autoStop = false;
    /// 主线程id(use_caller)
    pid_t m_rootThreadId = 0;
};


}
