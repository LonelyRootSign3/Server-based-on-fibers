#include "HttpConnection.h"
#include "../Log.h"
#include "../IOManager.h"
#include "../Util.h" // 获取时间等工具函数
#include <strings.h> // 用于 strcasecmp

namespace DYX {
namespace http {

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::Ptr sock, bool owner)
    : SocketStream(sock, owner) {
}

HttpConnection::~HttpConnection() {
    STREAM_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}

int HttpConnection::sendRequest(HttpRequest::Ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writefixsize(data.c_str(), data.size());
}

HttpResponse::Ptr HttpConnection::recvResponseHeader(std::shared_ptr<HttpResponseParser>& parser) {
    parser.reset(new HttpResponseParser);
    // 【修改 1】：直接使用宏或硬编码定义 Buffer 大小 (4096 是标准的页大小)
    uint64_t buff_size = 4096; 
    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr){ delete[] ptr; });
    char* data = buffer.get();
    int offset = 0;
    
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        data[len] = '\0';
        
        size_t nparse = parser->execute(data, len, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    
    return parser->getData();
}

HttpResponse::Ptr HttpConnection::recvResponse() {
    HttpResponseParser::Ptr parser(new HttpResponseParser);
    // 【修改 2】：使用固定的 Buffer 大小
    uint64_t buff_size = 4096; 
    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr){ delete[] ptr; });
    char* data = buffer.get();
    int offset = 0;
    
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        data[len] = '\0';
        size_t nparse = parser->execute(data, len, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);

    std::string body;
    auto rsp = parser->getData();
    
    // 【修改 3】：不使用底层 getParser().chunked，直接通过 HTTP 响应头判断 Transfer-Encoding
    bool is_chunked = false;
    std::string te = rsp->getHeader("Transfer-Encoding"); // 假设你的 HttpResponse 有 getHeader 方法
    if (strcasecmp(te.c_str(), "chunked") == 0) {
        is_chunked = true;
    }

    if(is_chunked) {
        STREAM_LOG_WARN(g_logger) << "RecvResponse with chunked data into std::string (Risk of OOM)";
        close();
    } else {
        // 【修改 4】：不使用 getContentLength()，直接解析 Content-Length 响应头
        int64_t length = 0;
        std::string cl_str = rsp->getHeader("Content-Length");
        if (!cl_str.empty()) {
            try {
                length = std::stoll(cl_str);
            } catch (...) {
                length = 0;
            }
        }

        if(length > 0) {
            body.resize(length);
            int len = 0;
            if(length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            if(length > 0) {
                if(readfixsize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
        }
    }
    rsp->setBody(body);
    return rsp;
}

int HttpConnection::readResponseBody(void* buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return read(buffer, length);
}

HttpResult::Ptr HttpConnection::DoGet(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::Ptr uri = Uri::Create(url);
    if(!uri) return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url");
    return DoRequest(http_method::HTTP_GET, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnection::DoGet(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return DoRequest(http_method::HTTP_GET, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnection::DoPost(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::Ptr uri = Uri::Create(url);
    if(!uri) return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url");
    return DoRequest(http_method::HTTP_POST, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnection::DoPost(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return DoRequest(http_method::HTTP_POST, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnection::DoRequest(http_method method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::Ptr uri = Uri::Create(url);
    if(!uri) return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url");
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnection::DoRequest(http_method method, Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    HttpRequest::Ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setClose(false);

    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::Ptr HttpConnection::DoRequest(HttpRequest::Ptr req, Uri::Ptr uri, uint64_t timeout_ms) {
    Address::Ptr addr = uri->createAddress();
    if(!addr) return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host");
    
    Socket::Ptr sock = Socket::CreateTCP(addr);
    if(!sock) return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr, "create sock fail");
    
    if(!sock->connect(addr, timeout_ms)) return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail");
    
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::Ptr conn = std::make_shared<HttpConnection>(sock);
    
    int rt = conn->sendRequest(req);
    if(rt == 0) return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "peer close");
    if(rt < 0) return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send error");
    
    auto rsp = conn->recvResponse();
    if(!rsp) return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv timeout");
    
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


// ============== HttpConnectionPool 实现 ==============
HttpConnectionPool::~HttpConnectionPool(){
    if(m_timer) {
        m_timer->cancel();
    }
    // 释放池中遗留的连接
    for(auto conn : m_conns) {
        delete conn;
    }
    m_conns.clear();
}
HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, bool is_https, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request)
    : m_host(host), m_vhost(vhost), m_port(port), m_maxSize(max_size), m_maxAliveTime(max_alive_time), m_maxRequest(max_request), m_isHttps(is_https) {
}

HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri, const std::string& vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request) {
    Uri::Ptr turi = Uri::Create(uri);
    if(!turi) {
        STREAM_LOG_ERROR(g_logger) << "invalid uri=" << uri;
        return nullptr;
    }
    HttpConnectionPool::ptr pool(new HttpConnectionPool(turi->getHost(), vhost, turi->getPort(), turi->getScheme() == "https", max_size, max_alive_time, max_request));

    // 定时器后台清理过期连接
    // 【修改 4】：使用 weak_ptr 防止内存泄漏，并把定时器存进 m_timer
    if(DYX::IOManager::GetThis()) {
        pool->m_timer = DYX::IOManager::GetThis()->addTimer(30000, [wp = std::weak_ptr<HttpConnectionPool>(pool)]() {
            // 定时器触发时，先尝试提升为强引用
            auto p = wp.lock();
            if(p) {
                p->cleanExpiredConnections();
            }
        }, true);
    }
    return pool;
}

HttpConnection::Ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = DYX::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* ptr = nullptr;
    
    {
        MutexType::Lock lock(m_mutex);
        while(!m_conns.empty()) {
            auto conn = *m_conns.begin();
            m_conns.pop_front();
            if(!conn->isConnected()) {
                invalid_conns.push_back(conn);
                continue;
            }
            if((conn->m_createTime + m_maxAliveTime) > now_ms) {
                ptr = conn;
                break;
            } else {
                invalid_conns.push_back(conn);
            }
        }
    }

    for(auto i : invalid_conns) delete i;
    m_total -= invalid_conns.size();

    if(!ptr) {
        IPAddress::Ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) return nullptr;
        addr->setPort(m_port);
        Socket::Ptr sock = Socket::CreateTCP(addr);
        if(!sock || !sock->connect(addr)) return nullptr;

        ptr = new HttpConnection(sock);
        ptr->m_createTime = DYX::GetCurrentMS();
        m_total++;
    }
    
    // 【漏洞加固】：捕获 shared_from_this 保证池生命周期
    return HttpConnection::Ptr(ptr, [pool_ptr = shared_from_this()](HttpConnection* p) {
        HttpConnectionPool::ReleasePtr(p, pool_ptr.get());
    });
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ptr->m_request++;
    if(!ptr->isConnected() || ((ptr->m_createTime + pool->m_maxAliveTime) <= DYX::GetCurrentMS()) || (ptr->m_request >= pool->m_maxRequest)) {
        delete ptr;
        pool->m_total--;
        return;
    }
    
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

void HttpConnectionPool::cleanExpiredConnections() {
    uint64_t now_ms = DYX::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;
    
    {
        MutexType::Lock lock(m_mutex); 
        auto it = m_conns.begin();
        while(it != m_conns.end()) {
            if(!(*it)->isConnected() || (((*it)->m_createTime + m_maxAliveTime) <= now_ms)) {
                invalid_conns.push_back(*it);
                it = m_conns.erase(it);
            } else {
                ++it;
            }
        }
    }
    for(auto conn : invalid_conns) delete conn;
    m_total -= invalid_conns.size();
}

HttpResult::Ptr HttpConnectionPool::doGet(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(http_method::HTTP_GET, url, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnectionPool::doGet(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(http_method::HTTP_GET, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(http_method::HTTP_POST, url, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnectionPool::doPost(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(http_method::HTTP_POST, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnectionPool::doRequest(http_method method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::Ptr uri = Uri::Create(url);
    if(!uri) return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url");
    return doRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::Ptr HttpConnectionPool::doRequest(http_method method, Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
    HttpRequest::Ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setClose(false);

    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::Ptr HttpConnectionPool::doRequest(HttpRequest::Ptr req, uint64_t timeout_ms) {
    auto conn = getConnection();
    if(!conn) return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "get conn fail");
    
    conn->getSocket()->setRecvTimeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if(rt == 0) return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "peer close");
    if(rt < 0) return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "socket error");
    
    auto rsp = conn->recvResponse();
    if(!rsp) return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv timeout");
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

}
}
