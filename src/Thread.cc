#include "Thread.h"
#include "Log.h"
#include "Util.h"
#include <cerrno>
#include <cstring>
#include <system_error>
//定义系统日志器
// static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");
namespace DYX{

//为每个线程创建一份独立的 _local_thread 指针和 _local_thread_name 字符串副本。
// 它们是线程局部存储（Thread Local Storage, TLS），即：
// 所有线程共享同一份代码；
// 但每个线程都有独立的变量副本；
// 各线程之间互不影响、互不干扰。

static thread_local Thread * _local_thread = nullptr;// thread-local pointer to current thread
static thread_local std::string _local_thread_name = "UNKNOWN";// thread-local name of current thread
// thread_local 的工作机制
// thread_local 的语义是：每个线程在第一次访问该变量时，系统会为它分配独立的存储空间。目的	说明
// 线程独立性	每个线程有自己的 _local_thread 和 _local_thread_name

// 避免竞争	不同线程互不干扰，不需要加锁
// 快速访问	比通过 pthread_getspecific 更高效（编译期优化）
// 全局可访问	在任何函数中都能通过 Thread::GetThis() 获取当前线程对象
// 生命周期绑定	变量在线程结束时自动销毁，无需手动清理




Thread::Thread(std::function<void()>cb , const std::string &name)
{
    _cb = cb;
    _name = name;
    if(name.empty()) {
        _name = "UNKNOWN";
    }
    int ret = pthread_create(&_thread, nullptr, &Thread::run, this);
    if(ret){
        // STREAM_LOG_ERROR(g_logger) << "pthread_create failed, ret=" << ret << ", errmsg=" << std::strerror(errno);
        throw std::runtime_error("pthread_create failed");
    }
    _sem.Wait();
}

Thread::~Thread()
{
    if(_thread)//若线程没有被回收，则分离线程
        pthread_detach(_thread);//分离线程，线程资源自动回收
}
//回收线程
void Thread::join()
{
    if(_thread){
        int ret = pthread_join(_thread, nullptr);
        if(ret){
            // STREAM_LOG_ERROR(g_logger) << "pthread_join failed, ret=" << ret << ", errmsg=" << std::strerror(errno);
            throw std::runtime_error("pthread_join failed");
        }
        _thread = 0;
    }
}


//TLS 变量，每个线程都有自己的拷贝，互不干扰。
//获取当前线程名(与thread_local相关)
std::string & Thread::getCurName()
{
    return _local_thread_name;
}
//获取当前线程对象(与thread_local相关)
Thread *Thread::getThis()
{
    return _local_thread;
}
//设置当前线程名(与thread_local相关)
void Thread::setCurName(const std::string &name)
{
    if(name.empty()) return;
    if(_local_thread){
        _local_thread->_name = name;
    }
    _local_thread_name = name;  
}


// 函数本身是共享的
// run 是静态成员函数，不依赖具体对象。
// 多个线程都会执行它。
// 每次调用使用的是独立的参数
// arg 指向不同的 Thread 实例。
// 所以 thread->_cb、thread->_name 都是各自对象的，不会互相干扰。
// 👉 结论：
// run 会被多个线程“同时执行”，但每个线程处理自己传入的那个 Thread 对象，不存在直接的重入冲突。


// run 是 多线程入口函数，会被多个线程同时执行。
// 但每个线程用的是不同的 Thread* arg，处理自己的数据。
// 靠 thread_local 保证线程局部状态独立，不会互相干扰。
// 所以可以说 run 是“可并发调用的”，但不是危险的“重入函数”。
void *Thread::run(void* arg)
{
    Thread* thread = (Thread*)arg;
    _local_thread = thread;
    _local_thread_name = thread->_name;
    _local_thread->_tid = DYX::GetThreadId();//获取内核级别的线程id

    // 设置操作系统层面（pthread/内核）的线程名，方便调试、top/ps 等工具显示。
    pthread_setname_np(pthread_self(), thread->_name.substr(0, 15).c_str());

    std::function<void()> call_back;
    call_back.swap(thread->_cb);
    // 将线程要执行的回调从 thread->_cb 移出到本地变量 cb：
    // swap 将 thread->m_cb 置为空（或默认构造状态），把原来的 callable 放入本地 cb。
    // 这样做的好处：释放 Thread 对象上对 m_cb 的持有（避免其它线程/析构中重复访问），并且是异常安全、开销小（O(1)）。
    // 等价替代写法： std::function<void()> cb = std::move(thread->m_cb);（也常用）。
    
    _local_thread->_sem.Notify();//唤醒主线程
    call_back();
    return 0;

}
// 使用了 thread_local 变量 (t_thread、t_thread_name) → 避免不同线程间冲突。
// 每个线程操作的 thread 实例不同 → 不共享状态。
// 唯一的全局状态是 g_logger，但一般日志器内部也会加锁，保证线程安全。
// 👉 所以 run 不是严格的可重入函数（因为依赖了 thread_local 和日志系统等外部状态），
// 但它是一个 线程安全函数，多个线程可以并发执行，不会出现数据竞争。

// run 函数在内存里只有一份，不会为每个线程复制一份。
// 但是每个线程都会“独立地执行它”，带着自己的参数和局部变量。
// 靠 局部变量 + TLS + 不同的 this，保证线程之间互不干扰。


}