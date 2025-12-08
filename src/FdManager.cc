#include "FdManager.h"
#include "Hook.h"
#include "Singleton.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


namespace DYX{
    
FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_close(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
        init();
    }
FdCtx::~FdCtx() {}

bool FdCtx::init(){
    if(m_isInit){
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;
    struct stat fd_stat;
    //获取数据元信息
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        //判断是否为socket
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    //判断是否为非阻塞
    if(m_isSocket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);//获取文件状态标志
        if(!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }
    m_userNonblock = false;
    return m_isInit;

}

void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}
  
FdCtx::Ptr FdManager::get(int fd, bool auto_create) {
    if (fd == -1) {
        return nullptr;
    }

    // ==== 第一阶段：读锁检查 ====
    {
        RWMutexType::RLock rlock(m_mutex);

        // fd 在范围内
        if (fd < (int)m_datas.size()) {
            auto ctx = m_datas[fd];

            // 已存在，直接返回
            if (ctx) {
                return ctx;
            }

            // 不自动创建，但位置为空 → 返回 nullptr
            if (!auto_create) {
                return nullptr;
            }

            // auto_create = true，且 ctx 为空
            // 需要创建 → 后面写锁处理
        }else {
            // fd 不在范围内，如果不自动创建，直接返回
            if (!auto_create) {
                return nullptr;
            }
            // auto_create = true 且需要扩容 → 写锁创建
        }
    }

    // ==== 第二阶段：写锁（创建） ====
    RWMutexType::WLock wlock(m_mutex);
    // double-check: 可能其他线程已创建
    if (fd < (int)m_datas.size() && m_datas[fd]) {
        return m_datas[fd];
    }

    // 确保容量能容纳 fd
    if (fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5 + 1);
    }

    // 创建并存储上下文
    auto ctx = std::make_shared<FdCtx>(fd);
    m_datas[fd] = ctx;

    return ctx;
}

void FdManager::del(int fd){
    if(fd == -1) return;
    RWMutexType::WLock lock(m_mutex);
    if((int)m_datas.size() <= fd){
        return;
    }
    m_datas[fd].reset();
}


}
