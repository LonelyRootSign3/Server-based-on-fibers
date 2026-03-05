#include "Tcp_server.h"
#include "Log.h"
namespace DYX{

static ConfigVar<uint64_t>::Ptr g_tcp_server_recv_timeout = 
    ConfigManager::Lookup<uint64_t>("tcp_server.recv_timeout" , 1000 * 60 * 2 , "tcp server recv timeout");

static Logger::Ptr g_logger = GET_NAME_LOGGER("system");

TcpServer::TcpServer( IOManager *acceptWorker 
                    , IOManager *ioWorker
                    , IOManager *businessWorker)
    :m_acceptWorker(acceptWorker)
    ,m_ioWorker(ioWorker)
    ,m_businessWorker(businessWorker)
    ,m_recvTimeout(g_tcp_server_recv_timeout->getValue())
    ,m_name("tcp_fiber_server/1.0.0")
    ,m_isStop(true) {}

TcpServer::~TcpServer() {
    // stop();
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

void TcpServer::setConfig(const TcpServerConf& v) {
    m_config.reset(new TcpServerConf(v));
}

bool TcpServer::bind(Address::Ptr addr, bool ssl){
    std::vector<Address::Ptr> addrs;
    std::vector<Address::Ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}

bool TcpServer::bind(const std::vector<Address::Ptr> &addrs
                    ,std::vector<Address::Ptr> &fails
                    ,bool ssl){
    m_ssl = ssl;
    for(auto &addr : addrs){
        // Socket::Ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        Socket::Ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)){
            fails.push_back(addr);
            STREAM_LOG_ERROR(g_logger)<<"bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            continue;
        }
        if(!sock->listen()){
            fails.push_back(addr);
            STREAM_LOG_ERROR(g_logger)<<"listen fail errno="
            << errno << " errstr=" << strerror(errno)
            << " addr=[" << addr->toString() << "]";
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()){
        m_socks.clear();
        return false;
    } 
    return true;
}
void TcpServer::handleClient(Socket::Ptr client){
    STREAM_LOG_INFO(g_logger)<<"handleClient client = "<<client->toString();
}
void TcpServer::startAccept(Socket::Ptr listen_sock){
    while(!m_isStop){
        Socket::Ptr client = listen_sock->accept();
        if(client){
            // STREAM_LOG_INFO(g_logger)<<"accept client = "<<client->toString();
            client->setRecvTimeout(m_recvTimeout);
            m_ioWorker->pushtask(std::bind(&TcpServer::handleClient
                                ,shared_from_this(),client));
        }else{
            STREAM_LOG_ERROR(g_logger)<<"accept error errorno = "<<errno
            <<strerror(errno);
        }
    }
}

bool TcpServer::start(){
    if(!m_isStop) return true;
    m_isStop = false;
    for(auto &listen_socks : m_socks){
        m_acceptWorker->pushtask(std::bind(&TcpServer::startAccept
                                    ,shared_from_this(),listen_socks));
    }
    return true;
}

void TcpServer::stop(){
    if(m_isStop) return;
    m_isStop = true;
    auto self = shared_from_this();
// 当 lambda 表达式以 [this, self] 的方式按值捕获 self 时：
// lambda 对象内部会生成一个 self 的拷贝。
// 这个拷贝动作会让 TcpServer 对象的引用计数（Reference Count）加 1。
// 只要这个 lambda 任务还在 m_acceptWorker 的队列里排队，或者正在被执行，lambda 对象本身就存活着，捕获的 self 也就存活着，从而强行保证了 TcpServer 对象不会被提前析构。
// 当 lambda 异步执行完毕并被销毁时，捕获的 self 随之销毁，引用计数减 1。如果此时没有其他人再使用 TcpServer，它才会被安全、合法地释放。
    m_acceptWorker->pushtask([this,self](){
        for(auto &listen_socks : m_socks){
            listen_socks->cancelAll();
            listen_socks->close();
        }
        m_socks.clear();
    });

}

}
