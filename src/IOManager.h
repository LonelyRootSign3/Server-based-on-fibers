#pragma once

#include "Schedule1.h"
#include "Timer.h"
#include <sys/epoll.h>
#include <string>
#if 1
namespace DYX{

class IOManager : public Scheduler ,public TimerManager{
public:
    using Ptr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;
    enum EVENT{
        NONE = 0x00,
        READ = 0x01,//读事件EPOLLIN
        WRITE = 0x04,//写事件EPOLLOUT
    };

private:
    //文件描述符的上下文信息
    struct FdContext{
        using MutexType = Mutex;

        struct EventContext{
            Scheduler *scheduler = nullptr;
            Fiber::Ptr fiber;
            std::function<void()> cb;//用于协程的回调方法
        };
        //指定事件类型，获取对应事件上下文
        EventContext &getContext(EVENT event);
        //重置特定事件上下文属性
        void resetContext(EventContext &context);
        //触发对应事件的回调（接收到idle唤醒的的信号后）
        void triggerEvent(EVENT event);


        EventContext read_event;    //读事件上下文
        EventContext write_event;   //写事件上下文
        int fd = 0;                 //对应文件描述符
        EVENT event_type = NONE;    //关注的事件类型
        MutexType mtx;

    };

public:

    IOManager(int threads = 1,bool use_main = true,const std::string &name = "IOManager");
    ~IOManager();
    
    //增添事件，在特定文件描述符上增添关注特定事件，设置触发该事件的回调函数
    bool addEvent(int fd ,EVENT event,std::function<void()> = nullptr);
    //删除事件，在特定文件描述符上删除关注特定事件
    bool delEvent(int fd ,EVENT event);
    //取消事件，在特定文件描述符上取消关注特定事件
    bool cancelEvent(int fd ,EVENT event);
    //取消所有事件，在特定文件描述符上取消关注所有事件
    bool cancelAll(int fd );

    static IOManager *GetThis();

protected:
    //通知协程，调度器有任务了
    virtual void Notify() override;
    //返回是否可以停止
    virtual bool CanStop() override;
    // 协程无任务可调度时执行idle协程
    virtual void Idle() override;
    // 重置socket句柄上下文的容器大小
    void resizeContext(int n);

    void Haslatest() override;//通知定时器管理器有新的定时器插入
    
private:
    int m_epfd = -1;        //epoll红黑树的句柄
    int m_eventfd_notify = -1;     //事件通知回调的句柄（eventfd系统调用）
    RWMutexType m_mutex;
    std::atomic<size_t> m_pendingCnt = {0};       //等待执行的事件数
    std::vector<FdContext *> m_fd_contexts; //句柄所对应的上下文的数组

};

}
#endif
