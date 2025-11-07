#pragma once

#include "Mutex.h"
#include "Nocopyable.h"
#include <string>
namespace DYX{

//线程类,继承自不可拷贝类
class Thread : Nocopy{
public:
    using Ptr = std::shared_ptr<Thread>;
    Thread(std::function<void()>cb , const std::string &name);
    ~Thread();

    //返回线程id
    pid_t Gettid(){ return _tid; }
    //返回线程名
    const std::string &GetName(){ return _name; }
    //回收线程
    void join();


    //获取当前线程名(与thread_local相关)
    static std::string &getCurName();
    //获取当前线程对象(与thread_local相关)
    static Thread *getThis();
    //设置当前线程名(与thread_local相关)
    static void setCurName(const std::string &name);
private:
    //线程入口函数,隶属整个thread类
    static void *run(void *arg);
private:
    pid_t _tid = -1;               //线程id
    pthread_t _thread = -1;        //线程句柄
    std::string _name;             //线程名
    Semaphore _sem;                //信号量 ,用于保证线程构造与启动同步
    std::function<void()> _cb;     //线程回调函数(实际要运行的函数)
};

}