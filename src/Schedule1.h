#pragma once
#include "Thread.h"
#include "Mutex.h"
#include "Fiber1.h"
#include <vector>
#include <string>
namespace DYX{

class Scheduler{
public:
    using MutexType = RWMutex;//读写互斥锁
    Scheduler(int threadnum,bool usemain = true,const std::string &name = " ");
    virtual ~Scheduler();

// 单个任务：万能引用 + 完美转发
    template<class FuncOrFiber>
    void pushtask(FuncOrFiber &&task, pid_t thread_id = -1) {
        bool need_notify = false;
        {
            MutexType::WLock lock(m_mutex);
            need_notify = pushtaskNolock(std::forward<FuncOrFiber>(task), thread_id);
        }
        if (need_notify) {
            Notify();
        }
    }

    // 批量任务：容器元素是 Fiber::Ptr 或 std::function<void()>
    template<class TasksIterator>
    void pushtasks(TasksIterator begin, TasksIterator end) {
        bool need_notify = false;
        {
            MutexType::WLock lock(m_mutex);
            for (; begin != end; ++begin) {
                need_notify = pushtaskNolock(*begin, -1) || need_notify;
            }
        }
        if (need_notify) {
            Notify();
        }
    }
    void Start();//启动调度器
    void Run();//调度器运行
    void Stop();//停止调度器
    
    static Scheduler* GetThis();//获取当前线程的调度
protected:
    virtual void Notify();//通知协程调度器有任务了
    virtual void Idle();//协程无任务可调度时，执行idle协程的入口函数
    virtual bool CanStop();//返回是否可以停止调度器
    bool hasIdleThreads() const { return m_idleThdCnt > 0; }//是否有空闲线程
private:
// ---- Task 改成“值语义” ----
    struct Task {
        Fiber::Ptr          fiberptr;   // 协程
        std::function<void()> cb;       // 回调
        pid_t               thread_id = -1;

        Task() = default;

        Task(Fiber::Ptr f, pid_t id = -1)
            : fiberptr(std::move(f))
            , thread_id(id) {
        }

        Task(std::function<void()> f, pid_t id = -1)
            : cb(std::move(f))
            , thread_id(id) {
        }

        void Reset() {
            fiberptr.reset();
            cb = nullptr;
            thread_id = -1;
        }

        bool valid() const {
            return fiberptr || static_cast<bool>(cb);
        }
    };

    // 统一入口：万能引用 + 完美转发
    template<class FuncOrFiber>
    bool pushtaskNolock(FuncOrFiber &&task, pid_t thread_id) {
        return addTask(std::forward<FuncOrFiber>(task), thread_id);
    }

    // -------- addTask 重载区 --------
    // 1) Fiber::Ptr && 版本（右值，直接 move）
    bool addTask(Fiber::Ptr &&fiber, pid_t thread_id) {
        Task t(std::move(fiber), thread_id);
        m_tasks.push_back(std::move(t));
        return true;
    }

    // 2) Fiber::Ptr const& 版本（左值，需要拷贝智能指针，代价很小）
    bool addTask(const Fiber::Ptr &fiber, pid_t thread_id) {
        Task t(fiber, thread_id);
        m_tasks.push_back(std::move(t));
        return true;
    }

    // 3) std::function<void()> && 版本（右值，move）
    bool addTask(std::function<void()> &&cb, pid_t thread_id) {
        Task t(std::move(cb), thread_id);
        m_tasks.push_back(std::move(t));
        return true;
    }

    // 4) std::function<void()> const& 版本（左值，拷贝一份）
    bool addTask(const std::function<void()> &cb, pid_t thread_id) {
        Task t(cb, thread_id);
        m_tasks.push_back(std::move(t));
        return true;
    }
    // 5) Fiber::Ptr* 版本
    bool addTask(Fiber::Ptr* fiber, pid_t thread_id) {
        if(fiber && *fiber) {
            Task t(*fiber, thread_id);   // 拿走 Fiber
            m_tasks.push_back(std::move(t));
            fiber->reset();              //  关键：当场解绑
            return true;
        }
        return false;
    }

    // 6) std::function<void()>* 版本
    bool addTask(std::function<void()>* cb, pid_t thread_id) {
        if(cb && *cb) {
            Task t(std::move(*cb), thread_id);
            m_tasks.push_back(std::move(t));
            cb->operator=(nullptr);      // 或 cb->clear()
            return true;
        }
        return false;
    }

private:

    bool m_isRunning = false;//是否正在运行
    bool use_main = true;//是否使用主线程调度
    bool m_enable_stop = false;//使能停止

    int m_buildThdCnt = 0;//当前构建的线程数
    std::string m_name;//调度器的名字
    std::vector<Thread::Ptr> m_threads;//线程池
    std::vector<pid_t> m_threadIds;//线程id池
    std::list<Task> m_tasks;//任务队列
    MutexType m_mutex;//读写互斥锁

    pid_t m_rootThreadId = 0;//主线程id
    std::atomic_int m_activeThdCnt{0};//当前活动线程数
    std::atomic_int m_idleThdCnt{0};//当前空闲线程数
    Fiber::Ptr m_rootFiber = nullptr;//根协程（usemain = true时）
};

}
