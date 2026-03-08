#pragma once

#include "Http.h"
#include "picohttpparser.h"
#include <memory>

namespace DYX {
namespace http {

/**
 * @brief HTTP 请求解析器
 */
class HttpRequestParser {
public:
    using Ptr = std::shared_ptr<HttpRequestParser>;

    HttpRequestParser();

    /**
     * @brief 执行 HTTP 请求解析
     * @param data 数据缓冲区
     * @param len 缓冲区中有效数据的总长度
     * @param last_len 上一次解析时的长度（用于 picohttpparser 优化，避免重复扫描）
     * @return >0: 解析成功，返回成功解析的头部字节数
     * -1: 解析出错 (报文格式错误)
     * -2: 报文不完整，需要继续接收数据
     */
    int execute(char* data, size_t len, size_t last_len);

    /// @brief 是否解析出错
    bool hasError() const { return m_error; }
    
    /// @brief 是否解析完成 (头部解析完毕)
    bool isFinished() const { return m_finished; }
    
    /// @brief 获取解析生成的 HttpRequest 对象
    HttpRequest::Ptr getData() const { return m_request; }

private:
    HttpRequest::Ptr m_request; // 解析出的请求对象
    bool m_error;               // 错误标志
    bool m_finished;            // 完成标志
};

/**
 * @brief HTTP 响应解析器
 */
class HttpResponseParser {
public:
    using Ptr = std::shared_ptr<HttpResponseParser>;

    HttpResponseParser();

    /**
     * @brief 执行 HTTP 响应解析
     * @param data 数据缓冲区
     * @param len 缓冲区中有效数据的总长度
     * @param last_len 上一次解析时的长度
     * @return >0: 解析成功，返回成功解析的头部字节数
     * -1: 解析出错 (报文格式错误)
     * -2: 报文不完整，需要继续接收数据
     */
    int execute(char* data, size_t len, size_t last_len);

    /// @brief 是否解析出错
    bool hasError() const { return m_error; }
    
    /// @brief 是否解析完成 (头部解析完毕)
    bool isFinished() const { return m_finished; }
    
    /// @brief 获取解析生成的 HttpResponse 对象
    HttpResponse::Ptr getData() const { return m_response; }

private:
    HttpResponse::Ptr m_response; // 解析出的响应对象
    bool m_error;                 // 错误标志
    bool m_finished;              // 完成标志
};

} // namespace http
} // namespace DYX