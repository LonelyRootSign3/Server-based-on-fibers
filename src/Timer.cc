#include "Timer.h"
#include "Util.h"
namespace DYX{

bool Timer::Compare::operator()(const Timer::Ptr &t1,const Timer::Ptr &t2) const {
    if(!t1 && !t2){
        return false;
    }
    if(!t1){
        return true;
    }
    if(!t2){
        return false;
    }

    if(t1->m_next == t2->m_next){
        return t1.get() > t2.get();
    }
    return t1->m_next > t2->m_next;
}

Timer::Timer(uint64_t next) : m_next(next){}
Timer::Timer(uint64_t ms,std::function<void()> cb,bool repeat,TimerManager *manager)
    : m_ms(ms),m_cb(cb),m_repeat(repeat),m_manager(manager){
    m_next = DYX::GetCurrentMS() + ms;
}
bool Timer::refresh(){
    if(!m_cb){
        return false;
    }
    RWMutex::WLock lock(m_manager->m_mutex);
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);

    m_next = DYX::GetCurrentMS() + m_ms;
    m_manager->addtimer(shared_from_this(),lock);
    return true;
}

bool Timer::cancel(){
    if(!m_cb){
        return false;
    }
    RWMutex::WLock lock(m_manager->m_mutex);
    m_cb = nullptr;
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);
    return true;
}

bool Timer::reset(uint64_t ms,bool from_now){
    if(m_ms == ms && !from_now){
        return true;
    }
    RWMutex::WLock lock(m_manager->m_mutex);
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);

    m_ms = ms;
    m_next = (from_now ? DYX::GetCurrentMS() : m_next) + m_ms;
    m_manager->addtimer(shared_from_this(),lock);
    return true;
}


TimerManager::TimerManager() : m_previoustime(DYX::GetCurrentMS()){}
TimerManager::~TimerManager(){}

Timer::Ptr TimerManager::addTimer(uint64_t ms,std::function<void()> cb,bool repeat){
    Timer::Ptr timer = std::shared_ptr<Timer>(new Timer(ms,cb,repeat,this));
    RWMutexType::WLock lock(m_mutex);
    addtimer(timer,lock);
    return timer;
}

static void Condition(std::function<void()> cb,std::weak_ptr<void> weak_cond){
    auto it = weak_cond.lock();
    if(it){
        cb();
    }
}
Timer::Ptr TimerManager::addConditionTimer(uint64_t ms,std::function<void()> cb,
                    std::weak_ptr<void> weak_cond,bool repeat){
    return addTimer(ms,std::bind(&Condition,cb,weak_cond),repeat);
}


void TimerManager::addtimer(Timer::Ptr timer,RWMutexType::WLock &lock){
    auto it = m_timers.insert(timer).first;
    bool need_notify = (it == m_timers.begin()) && !m_notify;
    if(need_notify){
        m_notify = true;
    }
    lock.unlock();
    
    if(need_notify){
        Haslatest();
    }
}

uint64_t TimerManager::getNextTimer(){
    m_notify = false;
    RWMutexType::RLock lock(m_mutex);

    if(m_timers.empty()){
        return ~0ull;
    }
    auto it = m_timers.begin();
    uint64_t now = DYX::GetCurrentMS();
    if((*it)->m_next > now){
        return (*it)->m_next - now;
    }
    return 0;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs){
    uint64_t now = DYX::GetCurrentMS();
    {
        RWMutexType::RLock rlock(m_mutex);
        if(m_timers.empty()) return;
    }

    RWMutexType::WLock wlock(m_mutex);
    if(m_timers.empty()) return;
    
    std::vector<Timer::Ptr> expired_timers;
    bool rollover = detectClockRollover(now);

    if(!rollover && (*m_timers.begin())->m_next > now){//没有过期定时器
        return;
    }

    Timer::Ptr now_timer = std::shared_ptr<Timer>(new Timer(now));
    auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);//找到第一个 <= now 的定时器
    ++it;//找到第一个 > now 的定时器(因为insert按区间批量插入的范围是[begin,end)),最后一个不包含
    expired_timers.insert(expired_timers.begin(),m_timers.begin(),it);
    m_timers.erase(m_timers.begin(),it);//删除所有过期的定时器
    cbs.reserve(expired_timers.size());//预开辟空间
    for(auto &timer : expired_timers){
        if(timer->m_cb){
            cbs.push_back(timer->m_cb);
        }
        if(timer->m_repeat){
            timer->m_next = now + timer->m_ms;
            m_timers.insert(timer);
        }else{
            timer->m_cb = nullptr;
        }
    }


}


bool TimerManager::detectClockRollover(uint64_t now_ms){
    // 检测服务器时间是否被调后了且滞后1小时
    if(now_ms < m_previoustime && now_ms < m_previoustime - 60*60*1000){
        return true;
    }
    m_previoustime = now_ms;
    return false;
}

bool TimerManager::hasTimer(){
    RWMutexType::RLock lock(m_mutex);
    return !m_timers.empty();
}

}




