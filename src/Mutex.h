#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>

#include "Nocopyable.h"

namespace DYX{

class Semaphore : Nocopy{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    void Wait();//申请一个信号量
    void Notify();//释放一个信号量
private:
    sem_t _semaphore;
};

// 局部锁的模板实现(要求传入的 '互斥量类型' 必须实现lock和unlock方法)
template<class T>
class ScopedLockImpl {
public:
    // 构造函数
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }
    // 析构函数,自动释放锁
    ~ScopedLockImpl() {
        unlock();
    }
    // 加锁
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    // 解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;    //互斥量
    bool m_locked; //是否已上锁
};

// 局部读锁模板实现(传入的'读写锁类型'必须实现rdlock和unlock方法)
template<class T>
class ReadScopedLockImpl {
public:
    // 构造函数
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    // 析构函数,自动释放锁
    ~ReadScopedLockImpl() {
        unlock();
    }
    // 上读锁
    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    // 解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;    //互斥量
    bool m_locked; //是否已上锁
};


// 局部写锁模板实现(传入的'读写锁类型'必须实现wrlock和unlock方法)
template<class T>
class WriteScopedLockImpl {
public:
    // 构造函数
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    // 析构函数
    ~WriteScopedLockImpl() {
        unlock();
    }
    // 上写锁
    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    // 释放锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;    //互斥量
    bool m_locked; //是否已上锁
};

// 互斥量
class Mutex : Nocopy{
public:

using Lock = ScopedLockImpl<Mutex>;        //互斥锁

    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex); 
    }
private:
    pthread_mutex_t m_mutex;
};

// 读写互斥量
class RWMutex : Nocopy{
public:

using RLock = ReadScopedLockImpl<RWMutex>;     //读锁
using WLock = WriteScopedLockImpl<RWMutex>;    //写锁

    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

// 自旋互斥量
class SpinMutex : Nocopy{
public:
using Lock = ScopedLockImpl<SpinMutex>;     //自旋锁

    SpinMutex(){
        pthread_spin_init(&m_mutex, 0);
    }
    ~SpinMutex(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};


//原子互斥量
class CASMutex : Nocopy{
public:
using Lock = ScopedLockImpl<CASMutex>;       //原子锁

    CASMutex(){
        m_mutex.clear();
    }
    ~CASMutex(){//析构函数中什么都不做，因为 atomic_flag 是无资源对象。
    }
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;//原子状态
};

//测试用的互斥量
class TestMutex : Nocopy{
public:
using Lock = ScopedLockImpl<TestMutex>;       //测试锁

    TestMutex(){}
    ~TestMutex(){}
    void lock(){}
    void unlock(){}
};

}



