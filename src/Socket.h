#pragma once

#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "Address.h"
#include "Nocopyable.h"

namespace DYX {

class Socket : public std::enable_shared_from_this<Socket> ,Nocopy {
public:
    using Ptr = std::shared_ptr<Socket>;
    using Weak_Ptr = std::weak_ptr<Socket>;
    //传输层
    enum Type{
        TCP = SOCK_STREAM,//面向字节流
        UDP = SOCK_DGRAM//面向数据报
    };
    //网络层（ip层）
    enum Family{
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX
    };
    Socket(int family ,int type, int protocol = 0);
    virtual ~Socket();
   
    //socket句柄是否合法
    bool isValid() const;
    //获取socket错误码
    int getErrorno();

    /**
     * @brief 获取发送超时时间(毫秒)
     */
    int64_t getSendTimeout();

    /**
     * @brief 设置发送超时时间(毫秒)
     */
    void setSendTimeout(int64_t v);

    /**
     * @brief 获取接受超时时间(毫秒)
     */
    int64_t getRecvTimeout();

    /**
     * @brief 设置接受超时时间(毫秒)
     */
    void setRecvTimeout(int64_t v);

    /**
     * @brief 获取sockopt @see getsockopt
     */
    bool getOption(int level, int option, void* result, socklen_t* len);

    /**
     * @brief 获取sockopt模板 @see getsockopt
     */
    template<class T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    /**
     * @brief 设置sockopt @see setsockopt
     */
    bool setOption(int level, int option, const void* result, socklen_t len);

    /**
     * @brief 设置sockopt模板 @see setsockopt
     */
    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    /**
     * @brief 接收connect链接
     * @return 成功返回新连接的socket,失败返回nullptr
     * @pre Socket必须 bind , listen  成功
     */
    virtual Socket::Ptr accept();

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    virtual bool bind(const Address::Ptr addr);

    /**
     * @brief 连接地址
     * @param[in] addr 目标地址
     * @param[in] timeout_ms 超时时间(毫秒)
     */
    virtual bool connect(const Address::Ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听socket
     * @param[in] backlog 未完成连接队列的最大长度
     * @result 返回监听是否成功
     * @pre 必须先 bind 成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 关闭socket
     */
    virtual bool close();

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int send(const void* buffer, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int sendTo(const void* buffer, size_t length, const Address::Ptr to, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int sendTo(const iovec* buffers, size_t length, const Address::Ptr to, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(void* buffer, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recvFrom(void* buffer, size_t length, Address::Ptr from, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recvFrom(iovec* buffers, size_t length, Address::Ptr from, int flags = 0);

    /**
     * @brief 创建TCP Socket(满足地址类型)
     * @param[in] address 地址
     */
    static Socket::Ptr CreateTCP(DYX::Address::Ptr address);

    /**
     * @brief 创建UDP Socket(满足地址类型)
     * @param[in] address 地址
     */
    static Socket::Ptr CreateUDP(DYX::Address::Ptr address);

    /**
     * @brief 创建IPv4的TCP Socket
     */
    static Socket::Ptr CreateTCPSocket();

    /**
     * @brief 创建IPv4的UDP Socket
     */
    static Socket::Ptr CreateUDPSocket();

    /**
     * @brief 创建IPv6的TCP Socket
     */
    static Socket::Ptr CreateTCPSocket6();

    /**
     * @brief 创建IPv6的UDP Socket
     */
    static Socket::Ptr CreateUDPSocket6();

    /**
     * @brief 创建Unix的TCP Socket
     */
    static Socket::Ptr CreateUnixTCPSocket();

    /**
     * @brief 创建Unix的UDP Socket
     */
    static Socket::Ptr CreateUnixUDPSocket();

    /**
     * @brief 输出信息到流中
     */
    virtual std::ostream& dump(std::ostream& os) const;

    virtual std::string toString() const;


    /**
     * @brief 取消读
     */
    bool cancelRead();

    /**
     * @brief 取消写
     */
    bool cancelWrite();

    /**
     * @brief 取消accept
     */
    bool cancelAccept();

    /**
     * @brief 取消所有事件
     */
    bool cancelAll();
    //获取本地网络地址
    Address::Ptr getLocalAddress();
    //获取网络连接对端地址
    Address::Ptr getRemoteAddress();
    int getSocketFd() const { return m_sockfd; }
    int getType() const { return m_type; }
    int getFamily() const { return m_family; }
    int getProtocol() const { return m_protocol; }
    bool IsConnected() const { return m_isConnected;  }

protected:
    /**
     * @brief 设置socket相关选项
     */
    void setSockfd();

    /**
     * @brief 创建socket
     */
    void newSockfd();

    /** 
     * @brief 初始化sock
     */
    virtual bool init(int sock);

private:     
    int m_sockfd;//套接字句柄
    int m_type;//类型（面向字节流还是数据报）
    int m_family;//ipv4 ipv6 unix
    int m_protocol;//协议
    bool m_isConnected;//是否建立连接
                        
    Address::Ptr m_localAddress;//本地网络地址
    Address::Ptr m_remoteAddress;//远端网络地址 
 
};
/*
class SSLSocket : public Socket {
public:
    typedef std::shared_ptr<SSLSocket> Ptr;

    static SSLSocket::Ptr CreateTCP(DYX::Address::Ptr address);
    static SSLSocket::Ptr CreateTCPSocket();
    static SSLSocket::Ptr CreateTCPSocket6();

    SSLSocket(int family, int type, int protocol = 0);
    virtual Socket::Ptr accept() override;
    virtual bool bind(const Address::Ptr addr) override;
    virtual bool connect(const Address::Ptr addr, uint64_t timeout_ms = -1) override;
    virtual bool listen(int backlog = SOMAXCONN) override;
    virtual bool close() override;
    virtual int send(const void* buffer, size_t length, int flags = 0) override;
    virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    virtual int sendTo(const void* buffer, size_t length, const Address::Ptr to, int flags = 0) override;
    virtual int sendTo(const iovec* buffers, size_t length, const Address::Ptr to, int flags = 0) override;
    virtual int recv(void* buffer, size_t length, int flags = 0) override;
    virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
    virtual int recvFrom(void* buffer, size_t length, Address::Ptr from, int flags = 0) override;
    virtual int recvFrom(iovec* buffers, size_t length, Address::Ptr from, int flags = 0) override;

    bool loadCertificates(const std::string& cert_file, const std::string& key_file);
    virtual std::ostream& dump(std::ostream& os) const override;
protected:
    virtual bool init(int sock) override;
private:
    std::shared_ptr<SSL_CTX> m_ctx;
    std::shared_ptr<SSL> m_ssl;
};


*/
}

