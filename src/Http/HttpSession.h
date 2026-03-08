#pragma once

#include "SocketStream.h" 
#include "Http.h"          
#include "HttpParser.h"
#include <memory>

namespace DYX {
namespace http {

/**
 * @brief HTTP 会话封装
 * @details 负责从 Socket 中解析 HTTP 请求，并发送 HTTP 响应
 */
class HttpSession : public SocketStream {
public:
    using Ptr = std::shared_ptr<HttpSession>;

    /**
     * @brief 构造函数
     * @param sock 底层 Socket 对象智能指针
     * @param owner 是否托管 Socket 的生命周期 (默认 true，析构时会自动关闭 Socket)
     */
    HttpSession(Socket::Ptr sock, bool owner = true);

    /**
     * @brief 接收 HTTP 请求
     * @return 返回解析出的 HttpRequest 智能指针。如果发生错误或连接关闭，返回 nullptr
     */
    HttpRequest::Ptr recvRequest();

    /**
     * @brief 发送 HTTP 响应
     * @param rsp 准备好的 HTTP 响应对象
     * @return >0 表示发送成功；=0 表示对方关闭；<0 表示 Socket 异常
     */
    int sendResponse(HttpResponse::Ptr rsp);
};

} // namespace http
} // namespace DYX