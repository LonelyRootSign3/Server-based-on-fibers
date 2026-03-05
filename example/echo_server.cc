#include "../src/Tcp_server.h"
#include "../src/Bytearray.h"

class EchoServer : public DYX::TcpServer{
public:
    EchoServer(int type);
    void handleClient(DYX::Socket::Ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type){}

void EchoServer::handleClient(DYX::Socket::Ptr client){



    
}
int main(){



    return 0;
}