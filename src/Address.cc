#include "Address.h"
#include "Endian.h"
#include "Log.h"
#include <cstring>
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>

namespace DYX{

static Logger::Ptr g_logger = GET_NAME_LOGGER("system");

//计算主机掩码
template<class T>
static T CreateMask(uint32_t bits){
    //必须是整数且是无符号，防止undefined behavior
    static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
        "T must be unsinged integral");
    constexpr uint32_t width = sizeof(T) * 8;//地址宽度
    if(bits >= width){//ub
        // 掩码位数大于等于地址宽度，返回0
        std::__throw_logic_error("CreateMask bits >= width");
        return T(0);
    }
    return (1 << (width - bits)) - 1;
}

//计算二进制中1的个数
template<class T>
static uint32_t CountBits(T value){
    uint32_t ret = 0;
    for(; value;++ret){
        value &= value - 1;
    }
    return ret;
}
// 返回地址族
int Address::getFamily() const{
    return getAddr()->sa_family;
}

// 转成字符串（内部依赖 insert）
std::string Address::toString() const{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

//--------3.比较运算符----------
bool Address::operator<(const Address& rhs) const{
    int minlen = std::min(getAddrLen(),rhs.getAddrLen());
    int res = memcmp(getAddr(),rhs.getAddr(),minlen);
    if(res < 0) {
        return true;
    } else if(res > 0) {
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}
bool Address::operator==(const Address& rhs) const{
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}
bool Address::operator!=(const Address& rhs) const{
    return !(*this == rhs);
}

//--------1.静态工厂，查询接口----------
// 根据 sockaddr 创建具体 Address 对象
Address::Ptr Address::Create(const sockaddr* addr, socklen_t addrlen){
    Address::Ptr res;
    switch(addr->sa_family){
        case AF_INET:
            res.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            res.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        case AF_UNIX:
            res.reset(new UnixAddress());
            break;
        default:
            res.reset(new UnknownAddress(*addr));
            break;
    }
    return res;
}

// 域名 / host 解析，返回多个 Address
bool Address::Lookup(std::vector<Address::Ptr>& result,// 输出参数
                const std::string& host,
                int family,
                int type,
                int protocol){
    // 给它一个 host（比如 "www.baidu.com:80"、"127.0.0.1:8080"、"[2001:db8::1]:443"）
    // 调用系统的 getaddrinfo() 把 域名/字符串地址解析成一个 addrinfo 链表
    addrinfo hints, *results,*next;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    // 用于传入getaddrinfo()的参数
    std::string node;// 域名或 IP 字符串  
    const char *service = nullptr;// 服务端口字符串
    
    // 指向 host 字符串的开始和结束位置
    const char *begin = host.c_str();
    const char *end = host.c_str() + host.size();
    

    //尝试处理[ipv6]:port
    if(!host.empty() && host[0] == '['){
        const char *endipv6 = (const char *)memchr(begin+1,']',end-(begin+1));
        if(endipv6){
            node.assign(begin+1,endipv6);//[...] []内的ipv6地址
            //安全判断 endipv6 还要在 [begin,end)里
            const char *aft = endipv6+1;
            if(aft < end && *aft == ':' && aft+1 < end){//判断]:后面是否有port
                service = aft+1;
            }
        }
    }
    //再尝试处理普通 ip:port
    if(node.empty()){
        const char *endip = (const char *)memchr(begin,':',end-begin);
        if(endip){
            node.assign(begin,endip);//[:] 前的 ip 地址
            //安全判断 endip 还要在 [begin,end)里
            const char *aft = endip+1;
            if(aft < end){
                service = aft;
            }
        }
    }
    //最后兜底把整个host当作node 
    if(node.empty()) node = host;

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        STREAM_LOG_DEBUG(g_logger) << "Address::Lookup(" << host
            << ", " << family << ", " << type << ", " << protocol << ") error=" << gai_strerror(error);
        return false;
    }

    for(next = results; next != nullptr; next = next->ai_next){
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    }

    freeaddrinfo(results);// 释放 addrinfo 链表
    return !result.empty();
}

// 返回任意一个 Address(内部调用lookup)
Address::Ptr Address::LookupAny(const std::string& host,
                            int family,
                            int type,
                            int protocol){
    std::vector<Address::Ptr> result;
    if(Lookup(result, host, family, type, protocol)){
        return result[0];
    }
    return nullptr;
}

// 返回任意一个 IPAddress
std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(
                            const std::string& host,
                            int family,
                            int type,
                            int protocol){
    std::vector<Address::Ptr> result;
    if(Lookup(result, host, family, type, protocol)){
        for(auto& addr : result){
            auto it = std::dynamic_pointer_cast<IPAddress>(addr);
            if(it){
                return it;
            }
        }
    }
    return nullptr;
}

//---------2.网卡相关接口----------
// 获取所有网卡地址
bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::Ptr, uint32_t>>& result,//输出型参数，返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    int family){
    struct ifaddrs *ifaddrs ,*next;
    if(getifaddrs(&ifaddrs) != 0){
        STREAM_LOG_ERROR(g_logger)<<"GetInterface getifaddrs error"<<strerror(errno);
        return false;
    }
    try{
        for(next = ifaddrs;next;next = next->ifa_next){
            Address::Ptr addr;
            uint32_t prefix_len = ~0u;
            if(next->ifa_addr == nullptr) continue;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) continue;
            switch(next->ifa_addr->sa_family){
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBits(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBits(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }
            if(addr){
                result.insert(std::make_pair(next->ifa_name,std::make_pair(addr,prefix_len)));
            }
        }

    }catch(...){
        STREAM_LOG_ERROR(g_logger)<<"GetInterfaceAddresses getifaddrs error"<<strerror(errno);
        freeifaddrs(ifaddrs);
        return false;
    }

    freeifaddrs(ifaddrs);
    return !result.empty();

} 

// 获取指定网卡地址
bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::Ptr, uint32_t>>& result,//输出型参数
    const std::string& iface,//网卡名称
    int family){
    if(iface.empty() || iface == "*"){
        if(family == AF_INET || family == AF_UNSPEC){
            result.push_back(std::make_pair(Address::Ptr(new IPv4Address()),0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC){
            result.push_back(std::make_pair(Address::Ptr(new IPv6Address()),0u));
        }
        return true;
    }

    std::multimap<std::string,std::pair<Address::Ptr,uint32_t>> all_addrs;
    if(!GetInterfaceAddresses(all_addrs,family)){
        return false;
    }
    auto [l,r] = all_addrs.equal_range(iface);
    for(auto it = l;it != r;++it){
        result.push_back(it->second);
    }
    return !result.empty();
}

//IP address

IPAddress::Ptr IPAddress::Create(const char *address, uint16_t port ){
    addrinfo hints ,*results;
    memset(&hints,0,sizeof(addrinfo));
    hints.ai_flags = AI_NUMERICHOST;// 只解析数字地址
    hints.ai_family = AF_UNSPEC;// 不指定地址族，允许 IPv4/IPv6
    int error = getaddrinfo(address, NULL, &hints, &results); 
    if(error) {
        STREAM_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << gai_strerror(error);
        return nullptr;
    }
    try {
        IPAddress::Ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;//sun_path 的最大长度（Linux 下一般是 108）减 1 是为了给 '\0' 留空间

UnixAddress::UnixAddress(){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un,sun_path) + MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

void UnixAddress::setAddrLen(uint32_t v){
    m_length = v;
}
// 获取 UnixDomainSocket 路径(用于调试)
std::string UnixAddress::getPath() const{
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}
const sockaddr* UnixAddress::getAddr() const{
    return (const sockaddr*)&m_addr;
} 
sockaddr* UnixAddress::getAddr(){
    return (sockaddr*)&m_addr;
} 
socklen_t UnixAddress::getAddrLen() const{
    return m_length;
} 
std::ostream& UnixAddress::insert(std::ostream& os) const{
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sa_family = family;
}
UnknownAddress:: UnknownAddress(const sockaddr& addr){
    memcpy(&m_addr,&addr,sizeof(m_addr));
}
const sockaddr* UnknownAddress::getAddr() const{
    return (const sockaddr*)&m_addr;
}
sockaddr* UnknownAddress::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t UnknownAddress::getAddrLen() const{
    return sizeof(m_addr);
}
std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os<<"[UnknownAddress family="<<m_addr.sa_family<<"]";
    return os;
}



IPv4Address::Ptr IPv4Address::Create(const char *address,
                                     uint16_t port ){
    IPv4Address::Ptr rt(new IPv4Address);
    rt->m_addr.sin_port = port;
    int res = inet_pton(AF_INET,address,&rt->m_addr.sin_addr);
    if(res <= 0){
        STREAM_LOG_ERROR(g_logger)<<"IPv4Address::Create("<<address<<","<<port<<") error="<<strerror(errno);
        return nullptr;
    }
    return rt;
}

// 从 sockaddr_in 构造
IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr = address;
}

// 从二进制 IP + port 构造
IPv4Address::IPv4Address(uint32_t address,
                         uint16_t port){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const{
    return (const sockaddr*)&m_addr;
}
sockaddr* IPv4Address::getAddr(){
    return (sockaddr*)&m_addr;
} 
socklen_t IPv4Address::getAddrLen() const{
    return sizeof(m_addr);
} 

//将 IPv4Address 转换为字符串表示
std::ostream& IPv4Address::insert(std::ostream& os) const{
    char buf[INET_ADDRSTRLEN]; // 16
    const char* ret = inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    if(ret) {
        os << buf;
    } else {
        os << "<invalid_ipv4>";
    }
    os << ":" << ntohs(m_addr.sin_port);
    return os;  
}  

IPAddress::Ptr IPv4Address::broadcastAddress(uint32_t prefix_len){
    if(prefix_len >= 32) {
        STREAM_LOG_ERROR(g_logger)<<"ipv4 prefix_len out of range";
        return nullptr;
    }
    uint32_t mask = CreateMask<uint32_t>(prefix_len);
    sockaddr_in baddr( m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(mask);
    return IPv4Address::Ptr(new IPv4Address(baddr));
} 
IPAddress::Ptr IPv4Address::networkAddress(uint32_t prefix_len){
    if(prefix_len >= 32) {
        STREAM_LOG_ERROR(g_logger)<<"ipv4 prefix_len out of range";
        return nullptr;
    }
    uint32_t mask = CreateMask<uint32_t>(prefix_len);
    sockaddr_in naddr( m_addr);
    naddr.sin_addr.s_addr &= byteswapOnLittleEndian(mask);
    return IPv4Address::Ptr(new IPv4Address(naddr));
}
IPAddress::Ptr IPv4Address::subnetMask(uint32_t prefix_len){
    if(prefix_len >= 32) {
        STREAM_LOG_ERROR(g_logger)<<"ipv4 prefix_len out of range";
        return nullptr;
    }
    uint32_t mask = CreateMask<uint32_t>(prefix_len);
    sockaddr_in subaddr;
    memset(&subaddr,0,sizeof(subaddr));
    subaddr.sin_family = AF_INET;  
    subaddr.sin_addr.s_addr &= ~(byteswapOnLittleEndian(mask));
    return IPv4Address::Ptr(new IPv4Address(subaddr));
} 

uint32_t IPv4Address::getPort() const{
    return byteswapOnLittleEndian(m_addr.sin_port);
} 
void IPv4Address::setPort(uint16_t v){
    m_addr.sin_port = byteswapOnLittleEndian(v);
} 

IPv6Address::Ptr IPv6Address::Create(const char *address,
                        uint16_t port ){
    IPv6Address::Ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int res = inet_pton(AF_INET6,address,&rt->m_addr.sin6_addr);
    if(res <= 0){
        STREAM_LOG_ERROR(g_logger)<<"IPv6Address::Create("<<address<<","<<port<<") error="<<strerror(errno);
        return nullptr;
    }
    return rt;


}
IPv6Address::IPv6Address(){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}  
IPv6Address::IPv6Address(const sockaddr_in6& address){
    m_addr = address;
}
IPv6Address::IPv6Address(const uint8_t address[16],
                         uint16_t port ){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}


const sockaddr* IPv6Address::getAddr() const{
    return (const sockaddr*)&m_addr;
}
sockaddr* IPv6Address::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t IPv6Address::getAddrLen() const{
    return sizeof(m_addr);
}

//将 IPv6Address 转换为字符串表示
std::ostream& IPv6Address::insert(std::ostream& os) const{
    char buf[INET6_ADDRSTRLEN]; // 46
    const char* ret = inet_ntop(AF_INET6, &m_addr.sin6_addr, buf, sizeof(buf));
    if(ret) {
        os << "[" << buf << "]";
    } else {
        os << "[<invalid_ipv6>]";
    }
    os << ":" << ntohs(m_addr.sin6_port);
    return os;
}

IPAddress::Ptr IPv6Address::broadcastAddress(uint32_t prefix_len){
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len/8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len/8 + 1;i < 16;i++){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::Ptr(new IPv6Address(baddr));
} 
IPAddress::Ptr IPv6Address::networkAddress(uint32_t prefix_len){
    sockaddr_in6 naddr(m_addr);
    naddr.sin6_addr.s6_addr[prefix_len/8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len/8 + 1;i < 16;i++){
        naddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::Ptr(new IPv6Address(naddr));

}
IPAddress::Ptr IPv6Address::subnetMask(uint32_t prefix_len){
    sockaddr_in6 subaddr;
    memset(&subaddr, 0, sizeof(subaddr));
    subaddr.sin6_family = AF_INET6;
    subaddr.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subaddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::Ptr(new IPv6Address(subaddr));
}

uint32_t IPv6Address::getPort() const{
    return byteswapOnLittleEndian(m_addr.sin6_port);
} 
void IPv6Address::setPort(uint16_t v){
    m_addr.sin6_port = byteswapOnLittleEndian(v);
} 


std::ostream& operator<<(std::ostream& os, const Address& addr){
    return addr.insert(os);
}


    
}



