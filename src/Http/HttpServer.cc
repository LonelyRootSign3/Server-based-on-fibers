#include "HttpServer.h"
#include "../Log.h"


namespace DYX{
namespace http{

static Logger::Ptr g_logger = GET_NAME_LOGGER("HttpServer");
HttpServer::HttpServer(bool keep_alive
            , IOManager* acceptWorker
            , IOManager* ioWorker
            , IOManager* bussinessWorker)
            :TcpServer(acceptWorker, ioWorker, bussinessWorker)
            , m_keep_alive(keep_alive){}

void HttpServer::handleClient(Socket::Ptr client){
    STREAM_LOG_DEBUG(g_logger)<<" handleClient "<<client->toString();
    HttpSession::Ptr session = std::make_shared<HttpSession>(client);
    do{
        auto req = session->recvRequest();
        if(!req){
            STREAM_LOG_ERROR(g_logger)<<"recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << client->toString() << " keep_alive= " << m_keep_alive;
            break;
        }
        HttpResponse::Ptr resp = std::make_shared<HttpResponse>(req->getVersion(), req->isClose() || !m_keep_alive);
        resp->setHeader("dyx", "dyx/0.1");
        session->sendResponse(resp);
        if(req->isClose() || !m_keep_alive) break;//两者只要有一个不是长连接，就break

    }while(true);
    session->close();
}

}
}
