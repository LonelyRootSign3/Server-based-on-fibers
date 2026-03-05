#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
namespace DYX{

class IPAddress;

class Address { 
public:
    using Ptr = std::shared_ptr<Address>;
    virtual ~Address() {}
    virtual const sockaddr *getAddr() const = 0;//只读
    virtual sockaddr *getAddr() = 0;//可写
    virtual socklen_t getAddrLen() const = 0;
    //可读性输出地址
    virtual std::ostream& insert(std::ostream&) const = 0;

    //--------1.静态工厂，查询接口----------
    // 根据 sockaddr 创建具体 Address 对象
    static Address::Ptr Create(const sockaddr* addr, socklen_t addrlen);

    // 域名 / host 解析，返回多个 Address
    static bool Lookup(std::vector<Address::Ptr>& result,
                    const std::string& host,
                    int family = AF_INET,
                    int type = 0,
                    int protocol = 0);

    // 返回任意一个 Address
    static Address::Ptr LookupAny(const std::string& host,
                                int family = AF_INET,
                                int type = 0,
                                int protocol = 0);

    // 返回任意一个 IPAddress
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(
                                const std::string& host,
                                int family = AF_INET,
                                int type = 0,
                                int protocol = 0);

    //---------2.网卡相关接口----------
    // 获取所有网卡地址
    static bool GetInterfaceAddresses(
        std::multimap<std::string,
        std::pair<Address::Ptr, uint32_t>>& result,
        int family = AF_INET);

    // 获取指定网卡地址
    static bool GetInterfaceAddresses(
        std::vector<std::pair<Address::Ptr, uint32_t>>& result,
        const std::string& iface,
        int family = AF_INET);
    

    // 返回地址族
    int getFamily() const;

    // 转成字符串（内部依赖 insert）
    std::string toString() const;
    
    //--------3.比较运算符----------
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;


};


class IPAddress : public Address {
public:
    using Ptr = std::shared_ptr<IPAddress>;
    
    // 根据字符串 IP + 端口 创建 IPAddress
    static IPAddress::Ptr Create(const char *address, uint16_t port = 0);
    // 计算广播地址
    virtual IPAddress::Ptr broadcastAddress(uint32_t prefix_len) = 0;

    // 计算网络地址
    virtual IPAddress::Ptr networkAddress(uint32_t prefix_len) = 0;

    // 计算子网掩码
    virtual IPAddress::Ptr subnetMask(uint32_t prefix_len) = 0;

    // 获取端口号
    virtual uint32_t getPort() const = 0;

    // 设置端口号
    virtual void setPort(uint16_t v) = 0;
};


class UnixAddress : public Address {
public:
    using Ptr = std::shared_ptr<UnixAddress>;
    UnixAddress();                     // 默认
    UnixAddress(const std::string& path);

    void setAddrLen(uint32_t v);
    std::string getPath() const;
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknownAddress : public Address {
public:
    using Ptr = std::shared_ptr<UnknownAddress>;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};


class IPv4Address : public IPAddress {
public:
    using Ptr = std::shared_ptr<IPv4Address>;
    static IPv4Address::Ptr Create(const char *address,
                               uint16_t port = 0);

    // 从 sockaddr_in 构造
    IPv4Address(const sockaddr_in& address);

    // 从二进制 IP + port 构造
    IPv4Address(uint32_t address = INADDR_ANY,
                uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::Ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::Ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::Ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    using Ptr = std::shared_ptr<IPv6Address>;
    static IPv6Address::Ptr Create(const char *address,
                               uint16_t port = 0);
    IPv6Address();  // 默认构造

    IPv6Address(const sockaddr_in6& address);

    IPv6Address(const uint8_t address[16],
                uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::Ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::Ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::Ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;

};
//流式输出Address
std::ostream& operator<<(std::ostream& os, const Address& addr);

  
}
