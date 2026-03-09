#pragma once 
#include "Http.h"
#include "Uri.h"
#include "HttpParser.h"
#include "../Stream/SocketStream.h"
#include "../Mutex.h"
#include "../Timer.h"
namespace DYX{
namespace http{

//http响应结果

class HttpResult{
public:
    using Ptr = std::shared_ptr<HttpResult>;

    enum class Error{
        /// 正常
        OK = 0,
        /// 非法URL
        INVALID_URL = 1,
        /// 无法解析HOST
        INVALID_HOST = 2,
        /// 连接失败
        CONNECT_FAIL = 3,
        /// 连接被对端关闭
        SEND_CLOSE_BY_PEER = 4,
        /// 发送请求产生Socket错误
        SEND_SOCKET_ERROR = 5,
        /// 超时
        TIMEOUT = 6,
        /// 创建Socket失败
        CREATE_SOCKET_ERROR = 7,
        /// 从连接池中取连接失败
        POOL_GET_CONNECTION = 8,
        /// 无效的连接
        POOL_INVALID_CONNECTION = 9,
    };
    HttpResult(int _result, HttpResponse::Ptr _response, const std::string& _error)
        : result(_result), response(_response), error(_error) {}
    /// 错误码
    int result;
    /// HTTP响应结构体
    HttpResponse::Ptr response;
    /// 错误描述
    std::string error;
    /// 将请求结果转换为易读的字符串格式，方便打印日志和调试
    std::string toString() const;
};

class HttpConnectionPool;
class HttpConnection : public SocketStream{
friend class HttpConnectionPool;
public:
    using Ptr = std::shared_ptr<HttpConnection>;
    HttpConnection(Socket::Ptr sock, bool owner = true);
    ~HttpConnection();
    // --- 便捷静态方法（自动建立连接并请求，适用于单次短连接） ---
    static HttpResult::Ptr DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");
    static HttpResult::Ptr DoGet(Uri::Ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers = {}
                        , const std::string& body = ""); 
    static HttpResult::Ptr DoPost(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers = {}
                        , const std::string& body = "");    
    static HttpResult::Ptr DoPost(Uri::Ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers = {}
                        , const std::string& body = "");     
    static HttpResult::Ptr DoRequest(http_method method
                        , const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers = {}
                        , const std::string& body = "");
    static HttpResult::Ptr DoRequest(http_method method 
                        , Uri::Ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers = {}
                        , const std::string& body = "");
    static HttpResult::Ptr DoRequest(HttpRequest::Ptr req
                        , Uri::Ptr uri
                        , uint64_t timeout_ms);  
                        
    /**
     * @brief 接收完整响应 (将整个 Body 读入内存，适用于小文件/API)
     */                    
    HttpResponse::Ptr recvResponse();
    /**
     * @brief 【改进优化】流式 API：仅接收 Header 头
     */
    HttpResponse::Ptr recvResponseHeader(std::shared_ptr<HttpResponseParser>& parser);
    /**
     * @brief 【改进优化】流式 API：读取 Body 数据 (适用于大文件边下边存)
     */
    int readResponseBody(void* buffer, size_t length);

    int sendRequest(HttpRequest::Ptr req);  
private:
    uint64_t m_createTime = 0;//记录该连接对象的创建时间戳（通常是毫秒
    uint64_t m_request = 0;//已经处理（发送）过的 HTTP 请求总数
};

/**
 * @brief HTTP 客户端连接池
 */
class HttpConnectionPool : public std::enable_shared_from_this<HttpConnectionPool> {
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    // 【改进优化】高并发场景下使用轻量级自旋锁
    typedef SpinMutex MutexType; 
    ~HttpConnectionPool();

    static HttpConnectionPool::ptr Create(const std::string& uri, const std::string& vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request);

    HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, bool is_https, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request);

    HttpConnection::Ptr getConnection();
    // --- 基于连接池的快速请求 API ---
    HttpResult::Ptr doGet(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doGet(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doPost(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doPost(Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doRequest(http_method method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doRequest(http_method method, Uri::Ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    HttpResult::Ptr doRequest(HttpRequest::Ptr req, uint64_t timeout_ms);



    /**
     * @brief 【改进优化】清理超时和失效的连接
     */
    void cleanExpiredConnections();

private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;
    uint32_t m_maxAliveTime;
    uint32_t m_maxRequest;
    bool m_isHttps;

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};

    DYX::Timer::Ptr m_timer;
};


}//http
}//DYX
