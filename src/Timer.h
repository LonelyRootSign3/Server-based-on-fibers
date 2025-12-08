#pragma once
#include <memory>
#include <functional>
#include <set>
#include "Mutex.h"
namespace DYX{


class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
friend TimerManager;

public:
    using Ptr = std::shared_ptr<Timer>;
    bool refresh();//刷新定时器时间
    bool cancel();//取消定时器  
    bool reset(uint64_t ms,bool from_now = false);//重置定时器时间
private:
    Timer(uint64_t next);
    Timer(uint64_t ms,std::function<void()> cb,bool repeat,TimerManager *manager);

    //定时器比较函数 用于优先队列
    struct Compare{
        bool operator()(const Timer::Ptr &t1,const Timer::Ptr &t2) const;
    };
private:  
    uint64_t m_ms = 0;//定时器时间间隔(毫秒)
    uint64_t m_next = 0;//下一次触发时间(毫秒) 通常为当前时间+m_ms    
    bool m_repeat = false;//是否重复执行(周期性任务)
    std::function<void()> m_cb;//定时器回调函数
    TimerManager *m_manager = nullptr;//定时器管理器指针
};


class TimerManager{
friend class Timer;
public:
    using RWMutexType = RWMutex;
    TimerManager();
    virtual ~TimerManager();

    Timer::Ptr addTimer(uint64_t ms,std::function<void()> cb,bool repeat = false);
    Timer::Ptr addConditionTimer(uint64_t ms,std::function<void()> cb,std::weak_ptr<void> weak_cond,bool repeat = false);
    uint64_t getNextTimer();//获取下一个定时器执行的时间间隔(毫秒)
    bool hasTimer();//是否有定时器 

    void listExpiredCb(std::vector<std::function<void()> >& cbs);//获取所有过期的定时器回调函数

protected:
    virtual void Haslatest() = 0;//通知定时器管理器有新的定时器插入
private:
    //将公有逻辑抽离到私有方法中
    void addtimer(Timer::Ptr timer,RWMutexType::WLock &lock);
    // 检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);
    // 识别系统时钟是否被人为或意外地向后调整（例如系统管理员手动修改时间、NTP服务器同步导致时间回退等情况）。
    // 时钟回滚会对定时器系统造成严重影响，可能导致定时器执行顺序混乱或长时间不触发，因此需要专门的检测机制。
private:
    std::set<Timer::Ptr, Timer::Compare> m_timers;//定时器集合，模拟小根堆
    RWMutexType m_mutex;//读写锁(保证定时器集合的线程安全)
    bool m_notify = false;//是否需要通知(有新的定时器插入且该任务的时间是最早的)
    uint64_t m_previoustime = 0;//上一次执行时间(毫秒)
};




}
