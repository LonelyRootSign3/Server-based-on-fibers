#include "Mutex.h"
#include <errno.h>
#include <system_error>
namespace DYX{
Semaphore::Semaphore(uint32_t count)
{
    if(sem_init(&_semaphore, 0, count)){//0表示线程间共享
        throw std::system_error(errno, std::system_category(), "Semaphore init failed");
    }
}
Semaphore::~Semaphore()
{
    sem_destroy(&_semaphore);
}
void Semaphore::Wait()//申请一个信号量
{
    if(sem_wait(&_semaphore)){
        throw std::system_error(errno, std::system_category(), "Semaphore wait failed");
    }
}
void Semaphore::Notify()//释放一个信号量
{
    if(sem_post(&_semaphore)){
        throw std::system_error(errno, std::system_category(), "Semaphore post failed");
    }
}


}