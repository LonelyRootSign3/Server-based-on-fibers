#pragma once
#include "../Tcp_server.h"
#include "HttpSession.h"
#include "HttpServlet.h"
namespace DYX{

namespace http{

class HttpServer : public TcpServer{
public:
    using Ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keep_alive = false
            , IOManager* acceptWorker = IOManager::GetThis()
            , IOManager* ioWorker = IOManager::GetThis()
            , IOManager* bussinessWorker = IOManager::GetThis());

    void setDispatcher(ServletDispatcher::Ptr dispatcher) { m_dispatcher = dispatcher;}
    ServletDispatcher::Ptr getDispatcher() { return m_dispatcher;}
    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::Ptr client);
private:
    bool m_keep_alive;

    ServletDispatcher::Ptr m_dispatcher;
};
}

}
