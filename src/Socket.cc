#include "Socket.h"
#include "Address.h"
#include "FdManager.h"
#include "IOManager.h"
#include "Log.h"
#include "Macro.h"
#include "Hook.h"
#include <limits>
#include <netinet/tcp.h>

namespace DYX{  

static Logger::Ptr g_logger = GET_NAME_LOGGER("system");

  

Socket::Socket(int family ,int type, int protocol)
    :m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_sockfd(-1){
    }
Socket::~Socket(){
    Socket::close();
}


void Socket:: setSockfd(){
    int val = 1;
    setOption(SOL_SOCKET,SO_REUSEADDR,val);
    if(m_type == SOCK_STREAM && (m_family == AF_INET || m_family == AF_INET6)){
        setOption(IPPROTO_TCP,TCP_NODELAY,val);
    }
}
void Socket:: newSockfd(){
    m_sockfd = ::socket(m_family,m_type,m_protocol);
    if(MY_LIKELY(m_sockfd != -1)){
        setSockfd();
    }else{
        STREAM_LOG_ERROR(g_logger)<<"new sockfd error"<<strerror(errno);
    }
}
bool Socket:: init(int sock){
    FdCtx::Ptr ctx = FdMgr::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClose()){
        m_sockfd = sock;
        m_isConnected = true;
        setSockfd();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}




bool Socket::isValid() const{
    return m_sockfd >= 0;
}

int Socket::getErrorno(){
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET ,SO_ERROR ,&error ,&len)){
        error = errno;
    }
    return error;
}
Address::Ptr Socket:: getLocalAddress(){
    if(m_localAddress) return m_localAddress;
    
    Address::Ptr result;
    switch(m_family){
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen(); 
    if(getsockname(m_sockfd,result->getAddr(),&addrlen)){
        return Address::Ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX){
        UnixAddress::Ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);  
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return result;
}
Address::Ptr Socket:: getRemoteAddress(){
    if(m_remoteAddress) return m_remoteAddress;
    
    Address::Ptr result;
    switch(m_family){
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen(); 
    if(getpeername(m_sockfd,result->getAddr(),&addrlen)){
        return Address::Ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX){
        UnixAddress::Ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);  
        addr->setAddrLen(addrlen);
    }  
    m_remoteAddress = result;
    return result;

}

int64_t Socket::getSendTimeout(){
    FdCtx::Ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx){
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}
void Socket::setSendTimeout(int64_t v){
    struct timeval tv{int(v/1000) ,int(v%1000*1000)};
    FdCtx::Ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx){
        ctx->setTimeout(SO_SNDTIMEO,v);
    }
    setOption(SOL_SOCKET ,SO_SNDTIMEO ,tv);
}
int64_t Socket::getRecvTimeout(){
    FdCtx::Ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx){
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v){
    struct timeval tv{int(v/1000) ,int(v%1000*1000)};
    FdCtx::Ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx){
        ctx->setTimeout(SO_RCVTIMEO,v);
    }
    setOption(SOL_SOCKET ,SO_RCVTIMEO ,tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len){
    if(getsockopt(m_sockfd, level, option, result, len) == -1){
        STREAM_LOG_ERROR(g_logger) << "getOption sock=" << m_sockfd
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len){
    if(setsockopt(m_sockfd, level, option, result, len) == -1){
        STREAM_LOG_ERROR(g_logger) << "setOption sock=" << m_sockfd
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::Ptr Socket::accept(){
    Socket::Ptr newsock(new Socket(m_family,m_type,m_protocol));
    int new_sockfd = ::accept(m_sockfd,nullptr,nullptr);
    if(new_sockfd == -1){
        STREAM_LOG_ERROR(g_logger)<<"accept error errorno = "<<errno
        <<strerror(errno);
    }
    bool ret = newsock->init(new_sockfd);
    if(ret) return newsock;

    return nullptr;
}
bool Socket::bind(const Address::Ptr addr){
    if(!isValid()){
        newSockfd();
        if(MY_UNLIKELY(!isValid())) return false;
    }
    if(MY_UNLIKELY(addr->getFamily() != m_family)){
        STREAM_LOG_ERROR(g_logger)<<"bind family equal error";
        return false;  
    }
    UnixAddress::Ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        Socket::Ptr sock = Socket::CreateUnixTCPSocket();
        if(sock->connect(uaddr)) {
            return false;
        } else {
            DYX::Unlink(uaddr->getPath(), true);
        }
    } 
    if(::bind(m_sockfd,addr->getAddr(),addr->getAddrLen())){
        STREAM_LOG_ERROR(g_logger)<<" ::bind error ";
        return false;
    }
    getLocalAddress();
    return true;

}
bool Socket::connect(const Address::Ptr addr, uint64_t timeout_ms){
    if(!isValid()){
        newSockfd();
        if(MY_UNLIKELY(!isValid())) return false;
    }
    if(MY_UNLIKELY(addr->getFamily() != m_family)){
        STREAM_LOG_ERROR(g_logger)<<"connect family equal error";
        return false;  
    }
    if((uint64_t)-1 == timeout_ms){
        if(::connect(m_sockfd,addr->getAddr(),addr->getAddrLen())){
            STREAM_LOG_ERROR(g_logger)<<" ::connect error ";
            return false;
        }
    }else{
        if(::connect_with_timeout(m_sockfd,addr->getAddr(),addr->getAddrLen(),timeout_ms)){
            STREAM_LOG_ERROR(g_logger)<<" ::connect_with_timeout error ";
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms){
    if(!m_isConnected){
        if(!m_remoteAddress){
            STREAM_LOG_ERROR(g_logger)<<"reconnect error remote address is null";
            return false;
        }
        return connect(m_remoteAddress,timeout_ms);
    }
    return true;
}

bool Socket::listen(int backlog) {
    if(!isValid()) {
        STREAM_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    if(::listen(m_sockfd, backlog)) {
        STREAM_LOG_ERROR(g_logger) << "listen error errno=" << errno
            << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close(){
    if(!m_isConnected && m_sockfd == -1) {
        return true;
    }
    m_isConnected = false; 
    if(m_sockfd != -1) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
    return false;
}


int Socket::send(const void* buffer, size_t length, int flags){
    if(m_isConnected){
        return ::send(m_sockfd,buffer,length,flags);
    }
    return -1;
}
int Socket::send(const iovec* buffers, size_t length, int flags){
    if(m_isConnected){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers; 
        msg.msg_iovlen = length;
        return ::sendmsg(m_sockfd, &msg, flags);
    }
    return -1;
}
int Socket::sendTo(const void* buffer, size_t length, const Address::Ptr to, int flags){
    if(m_isConnected){
        return ::sendto(m_sockfd,buffer,length,flags,to->getAddr(),to->getAddrLen());
    }
    return -1;
}
int Socket::sendTo(const iovec* buffers, size_t length, const Address::Ptr to, int flags){
    if(m_isConnected){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers; 
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags){
    if(m_isConnected){
        return ::recv(m_sockfd,buffer,length,flags);
    }
    return -1;
}
int Socket::recv(iovec* buffers, size_t length, int flags){
    if(m_isConnected){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffers; 
        msg.msg_iovlen = length;
        return ::recvmsg(m_sockfd, &msg, flags);
    }
    return -1;
}
int Socket::recvFrom(void* buffer, size_t length, Address::Ptr from, int flags){
    if(m_isConnected){
        socklen_t addrlen = from->getAddrLen();
        return ::recvfrom(m_sockfd,buffer,length,flags,from->getAddr(),&addrlen);
    }
    return -1;
}
int Socket::recvFrom(iovec* buffers, size_t length, Address::Ptr from, int flags){
    if(m_isConnected){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffers; 
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

Socket::Ptr Socket::CreateTCP(DYX::Address::Ptr address){
    Socket::Ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}
Socket::Ptr Socket::CreateUDP(DYX::Address::Ptr address){
    Socket::Ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSockfd();
    sock->m_isConnected = true;
    return sock;
}
Socket::Ptr Socket::CreateTCPSocket(){
    Socket::Ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}
Socket::Ptr Socket::CreateUDPSocket(){
    Socket::Ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSockfd();
    sock->m_isConnected = true;
    return sock;
}
Socket::Ptr Socket::CreateTCPSocket6(){
    Socket::Ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}
Socket::Ptr Socket::CreateUDPSocket6(){
    Socket::Ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSockfd();
    sock->m_isConnected = true;
    return sock;
}
Socket::Ptr Socket::CreateUnixTCPSocket(){
    Socket::Ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}
Socket::Ptr Socket::CreateUnixUDPSocket(){
    Socket::Ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}
bool Socket:: cancelRead(){
   return IOManager::GetThis()->cancelEvent(m_sockfd,IOManager::READ); 
}
bool Socket:: cancelWrite(){
   return IOManager::GetThis()->cancelEvent(m_sockfd,IOManager::WRITE); 
}
bool Socket:: cancelAccept(){
   return IOManager::GetThis()->cancelEvent(m_sockfd,IOManager::READ); 
}
bool Socket:: cancelAll(){
   return IOManager::GetThis()->cancelAll(m_sockfd); 
}


std::ostream& Socket::dump(std::ostream& os) const{
    os << "[Socket sock=" << m_sockfd
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/*
SSLSocket::Ptr SSLSocket::CreateTCP(DYX::Address::Ptr address){}
SSLSocket::Ptr SSLSocket::CreateTCPSocket(){}
SSLSocket::Ptr SSLSocket::CreateTCPSocket6(){}

SSLSocket::SSLSocket(int family, int type, int protocol ) 
    :Socket(family,type,protocol){}
Socket::Ptr accept() {}
bool SSLSocket::bind(const Address::Ptr addr) {}
bool SSLSocket::connect(const Address::Ptr addr, uint64_t timeout_ms) {}
bool SSLSocket::listen(int backlog) {}
bool SSLSocket::close() {}
int SSLSocket::send(const void* buffer, size_t length, int flags) {}
int SSLSocket::send(const iovec* buffers, size_t length, int flags) {}
int SSLSocket::sendTo(const void* buffer, size_t length, const Address::Ptr to, int flags) {}
int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::Ptr to, int flags) {}
int SSLSocket::recv(void* buffer, size_t length, int flags) {}
int SSLSocket::recv(iovec* buffers, size_t length, int flags) {}
int SSLSocket::recvFrom(void* buffer, size_t length, Address::Ptr from, int flags) {}
int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::Ptr from, int flags) {}

bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file){}
std::ostream& SSLSocket::dump(std::ostream& os) const {}
*/
}
