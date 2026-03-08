#include "../Stream.h"
#include "../Socket.h"

namespace DYX{

class SocketStream : public Stream{
public:
    using Ptr = std::shared_ptr<SocketStream>;
    SocketStream(Socket::Ptr sock, bool owner = true);
    ~SocketStream();
    virtual int read(void *buf,size_t len) override;
    virtual int read(ByteArray::Ptr ba,size_t len) override;
    virtual int write(const void *buf,size_t len) override;
    virtual int write(ByteArray::Ptr ba,size_t len) override;
    virtual void close() override;

    


    Socket::Ptr getSocket() const {return m_socket;}   
    bool isConnected() const;

    Address::Ptr getRemoteAddress();
    Address::Ptr getLocalAddress();
    std::string getRemoteAddressString();
    std::string getLocalAddressString();
private:
    bool m_owner;//是否主控
    Socket::Ptr m_socket;//accept 或 connect成功的套接字
};
}
