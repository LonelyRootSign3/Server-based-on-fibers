#include "HttpParser.h"
#include "../Log.h"
#include <string.h>

namespace DYX {
namespace http {

static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

// ==========================================
// HttpRequestParser 实现
// ==========================================

HttpRequestParser::HttpRequestParser()
    : m_error(false)
    , m_finished(false) {
    m_request = std::make_shared<HttpRequest>();
}

int HttpRequestParser::execute(char* data, size_t len, size_t last_len) {
    const char *method, *path;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]); // 最大支持的 header 数量

    // 调用 picohttpparser 原生请求解析接口
    int ret = phr_parse_request(data, len, 
                                &method, &method_len, 
                                &path, &path_len,
                                &minor_version, 
                                headers, &num_headers, 
                                last_len);
    if (ret > 0) {
        m_finished = true;
        
        // 1. 设置 Method (复用 Http.cc 里的 StringToHttpMethod)
        std::string method_str(method, method_len);
        m_request->setMethod(StringToHttpMethod(method_str));

        // 2. 设置 Path 和 Query
        std::string full_path(path, path_len);
        size_t query_pos = full_path.find('?');
        if (query_pos != std::string::npos) {
            m_request->setPath(full_path.substr(0, query_pos));
            m_request->setQuery(full_path.substr(query_pos + 1));
        } else {
            m_request->setPath(full_path);
        }

        // 3. 设置 HTTP 版本 (1.0 = 0x10, 1.1 = 0x11)
        m_request->setVersion(minor_version == 1 ? 0x11 : 0x10);

        // 4. 设置 Headers
        for (size_t i = 0; i < num_headers; ++i) {
            m_request->setHeader(
                std::string(headers[i].name, headers[i].name_len),
                std::string(headers[i].value, headers[i].value_len)
            );
        }

        // 5. 初始化一些内部状态（比如 connection 标志位）
        m_request->init();

    } else if (ret == -1) {
        m_error = true;
        STREAM_LOG_ERROR(g_logger) << "HttpRequestParser: picohttpparser parsed invalid request";
    }
    
    // ret == -2 表示不完整，直接返回让外层继续 recv
    return ret; 
}


// ==========================================
// HttpResponseParser 实现
// ==========================================

HttpResponseParser::HttpResponseParser()
    : m_error(false)
    , m_finished(false) {
    m_response = std::make_shared<HttpResponse>();
}

int HttpResponseParser::execute(char* data, size_t len, size_t last_len) {
    int minor_version;
    int status;
    const char *msg;
    size_t msg_len;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    // 调用 picohttpparser 原生响应解析接口
    int ret = phr_parse_response(data, len, 
                                 &minor_version, 
                                 &status, 
                                 &msg, &msg_len, 
                                 headers, &num_headers, 
                                 last_len);
    if (ret > 0) {
        m_finished = true;
        
        // 1. 设置 HTTP 版本
        m_response->setVersion(minor_version == 1 ? 0x11 : 0x10);
        
        // 2. 设置状态码 (复用 Http.h 里的 http_status)
        m_response->setStatus((http_status)status);
        
        // 3. 设置状态描述 (Reason Phrase, 如 "OK", "Not Found")
        if (msg_len > 0) {
            m_response->setReason(std::string(msg, msg_len));
        }

        // 4. 设置 Headers
        for (size_t i = 0; i < num_headers; ++i) {
            m_response->setHeader(
                std::string(headers[i].name, headers[i].name_len),
                std::string(headers[i].value, headers[i].value_len)
            );
        }

    } else if (ret == -1) {
        m_error = true;
        STREAM_LOG_ERROR(g_logger) << "HttpResponseParser: picohttpparser parsed invalid response";
    }
    
    return ret;
}

} // namespace http
} // namespace DYX