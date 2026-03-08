#pragma once
#include "../Tcp_server.h"
#include "HttpSession.h"
namespace DYX{

namespace http{

class HttpServer : public TcpServer{
public:
    using Ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keep_alive = false
            , IOManager* acceptWorker = IOManager::GetThis()
            , IOManager* ioWorker = IOManager::GetThis()
            , IOManager* bussinessWorker = IOManager::GetThis());

protected:
    virtual void handleClient(Socket::Ptr client);
private:
    bool m_keep_alive;
};
}

}
