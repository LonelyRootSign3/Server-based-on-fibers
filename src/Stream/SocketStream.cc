#include "SocketStream.h"

namespace DYX{
SocketStream::SocketStream(Socket::Ptr sock, bool owner)
    :m_owner(owner),m_socket(sock){}
SocketStream::~SocketStream(){
    if(m_socket && m_owner){
        m_socket->close();
    }
}
int SocketStream::read(void *buf,size_t len) {
    return m_socket->recv(buf,len);
}
int SocketStream::read(ByteArray::Ptr ba,size_t len) {
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs,len);
    int ret = m_socket->recv(&iovs[0],iovs.size());//这个底层调的recvmsg
    if(ret > 0) {
        ba->setWritePosition(ba->getWritePosition() + ret);
    }
    return ret;
}
int SocketStream::write(const void *buf,size_t len) {
    return m_socket->send(buf,len);
}
int SocketStream::write(ByteArray::Ptr ba,size_t len) {
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs,len);
    int ret = m_socket->send(&iovs[0],iovs.size());//这个底层调的sendmsg
    if(ret > 0) {
        ba->setReadPosition(ba->getReadPosition() + ret);
    }
    return ret;
}
void SocketStream::close() {
    if(m_socket) m_socket->close();
}

bool SocketStream::isConnected() const{
    return m_socket && m_socket->IsConnected();
}

Address::Ptr SocketStream::getRemoteAddress(){
    if(m_socket) return m_socket->getRemoteAddress();
    return nullptr;
}
Address::Ptr SocketStream::getLocalAddress(){
    if(m_socket) return m_socket->getLocalAddress();
    return nullptr;
}
std::string SocketStream::getRemoteAddressString(){
    auto it = getRemoteAddress();
    if(it) return it->toString();
    return "";
}
std::string SocketStream::getLocalAddressString(){
    auto it = getLocalAddress();
    if(it) return it->toString();
    return "";
}

}


