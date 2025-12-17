#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "Mutex.h"
#include "Singleton.h"
#if 1
namespace DYX{

//文件描述符上下文类
class FdCtx : public std::enable_shared_from_this<FdCtx>{
public:
    using Ptr = std::shared_ptr<FdCtx>;

    FdCtx(int fd);
    ~FdCtx();
    bool init();
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isSysNonblock() const { return m_sysNonblock; }
    bool isUserNonblock() const { return m_userNonblock; }
    bool isClose() const { return m_close; }
    void setClose(bool v) { m_close = v; }

    void setUserNonblock(bool v) { m_userNonblock = v; }
    void setSysNonblock(bool v) { m_sysNonblock = v; }
private:
    bool m_isInit: 1;
    bool m_isSocket: 1;
    bool m_sysNonblock: 1;
    bool m_userNonblock: 1;
    bool m_close: 1;
    int m_fd;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

//文件描述符上下文类(这里只管理socket类，系统文件描述符不管理)
class FdManager{
public:
    using RWMutexType = RWMutex;
    FdManager();

    /// 获取/创建文件句柄类FdCtx (auto_create: 是否自动创建)
    FdCtx::Ptr get(int fd, bool auto_create = false);
    /// 删除文件句柄类
    void del(int fd);

private:
    /// 读写锁
    RWMutexType m_mutex;
    /// 文件句柄集合
    std::vector<FdCtx::Ptr> m_datas;
};

using FdMgr = Singleton<FdManager>;
    
}
#endif