#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>
#include <stdint.h>
namespace DYX{

namespace http{

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                  Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \
// 状态码枚举
enum http_status
{
#define XX(num, name, string) HTTP_STATUS_##name = num,
  HTTP_STATUS_MAP(XX)
#undef XX
};


/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \


// 请求方法枚举
enum http_method
{
#define XX(num, name, string) HTTP_##name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
};

//Src: 源类型，Tar: 目标类型
template<class Tar,class Src>
struct LexicalCast{//类型转换模板类 ->仿函数
    Tar operator()(const Src &src){
        std::stringstream ss;
        ss<<src;// 把源类型写入到流中
        Tar tar;
        ss>>tar;// 从流中读取到目标类型

        // 检查是否转换完整（例如 "123abc" 转 int 时会有残余）
        if (!ss.eof() || ss.fail()) {
            throw std::invalid_argument("LexicalCast: conversion failed");
        }
        return tar; 
    }
 
}; 
// 针对 string -> string 的特化（避免额外开销）
template<>  
struct LexicalCast<std::string, std::string> {
    std::string operator()(const std::string& v) {
        return v;
    }
};



http_method StringToHttpMethod(const std::string& m); 
http_method CharsToHttpMethod(const char* m);
const char *HttpMethodToString(http_method& m);
const char *HttpStatusToString(http_status& s);

struct CompareIngnoreCase {
    bool operator()(const std::string &lhs ,const std::string &rhs);
};

class HttpRequest{
public:
    using MapType = std::map<std::string, std::string, CompareIngnoreCase>;// 头字段、参数、Cookie 等的映射表类型
    using Ptr = std::shared_ptr<HttpRequest>;
//getter接口
    http_method getMethod() const { return m_method; }
    uint8_t getVersion() const { return m_version; }
    const std::string& getPath() const { return m_path; }
    const std::string& getQuery() const { return m_query; }
    const std::string& getBody() const { return m_body; }
    const MapType& getHeaders() const { return m_headers; }
    const MapType& getParams() const { return m_params; }
    const MapType& getCookies() const { return m_cookies; }
    bool isClose() const { return m_close; }
    bool isWebsocket() const { return m_websocket; }

    std::string getHeader(const std::string& key,
                      const std::string& def = "") const;
    std::string getParam(const std::string& key,
                     const std::string& def = "");
    std::string getCookie(const std::string& key,
                      const std::string& def = "");                 
//setter接口
    void setMethod(http_method v) { m_method = v; }
    void setVersion(uint8_t v) { m_version = v; }
    void setPath(const std::string& v) { m_path = v; }
    void setQuery(const std::string& v) { m_query = v; }
    void setFragment(const std::string& v) { m_fragment = v; }
    void setBody(const std::string& v) { m_body = v; }
    void setClose(bool v) { m_close = v; }
    void setWebsocket(bool v) { m_websocket = v; }
    void setHeaders(const MapType& v) { m_headers = v; }
    void setParams(const MapType& v) { m_params = v; }
    void setCookies(const MapType& v) { m_cookies = v; }

    void setHeader(const std::string& key,
               const std::string& val);
    void delHeader(const std::string& key);
    
    void setParam(const std::string& key,
              const std::string& val);
    void delParam(const std::string& key);
    
    void setCookie(const std::string& key,
               const std::string& val);
    void delCookie(const std::string& key);
    
    bool hasHeader(const std::string& key,
                  std::string* val = nullptr);

    bool hasParam(const std::string& key,
                  std::string* val = nullptr);

    bool hasCookie(const std::string& key,
                  std::string* val = nullptr);
private:
    http_method m_method;// 请求方法
    std::string m_path;  //请求路径
    std::string m_query;  //查询字符串
    std::string m_fragment;//锚点
    uint8_t m_version;// 请求版本

    MapType m_headers;// 请求报头字段
    MapType m_cookies;// Cookie 字段

    std::string m_body;    //请求正文 

    bool m_close;// 是否是短连接（自动关闭）
    bool m_websocket;// 是否是websocket连接(双向同时可以发信息，不只是单一请求应答)

    uint8_t m_parserParamFlag;//延迟解析参数 0x1-query 已解析   0x2-body 已解析   0x4-cookie 已解析，避免重复解析
    MapType m_params;// 查询参数

};


class HttpResponse{
public:
    using MapType = std::map<std::string, std::string, CompareIngnoreCase>;// 响应报头
    using Ptr = std::shared_ptr<HttpResponse>;
//getter接口
    http_status getStatus() const { return m_status; }
    uint8_t getVersion() const { return m_version; }
    const std::string& getBody() const { return m_body; }
    const std::string& getReason() const { return m_reason; }
    const MapType& getHeaders() const { return m_headers; }
    bool isClose() const { return m_close; }
    bool isWebsocket() const { return m_websocket; }
    std::string getHeader(const std::string& key,
                      const std::string& def = "") const;
//setter接口
    void setStatus(http_status v) { m_status = v; }
    void setVersion(uint8_t v) { m_version = v; }
    void setBody(const std::string& v) { m_body = v; }
    void setReason(const std::string& v) { m_reason = v; }
    void setHeaders(const MapType& v) { m_headers = v; }
    void setClose(bool v) { m_close = v; }
    void setWebsocket(bool v) { m_websocket = v; }
    
    void setHeader(const std::string& key,
               const std::string& val);
    void delHeader(const std::string& key);

    void setCookie(const std::string& key,
               const std::string& val,
               time_t expired = 0,
               const std::string& path = "",
               const std::string& domain = "",
               bool secure = false);
    void setRedirect(const std::string& uri);

private:
    uint8_t m_version;// http版本
    http_status m_status;// 响应状态码
    std::string m_reason;// 状态描述

    MapType m_headers;// 响应头字段
    std::vector<std::string> m_cookies;// Cookie 字段

    std::string m_body;    //响应正文 

    bool m_close;// 是否是短连接（自动关闭）
    bool m_websocket;// 是否是websocket连接(双向同时可以发信息，不只是单一请求应答)


};

}
}