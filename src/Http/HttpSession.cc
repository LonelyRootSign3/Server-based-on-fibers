#include "HttpSession.h"
#include <sstream>
#include <string>

namespace DYX {
namespace http {

HttpSession::HttpSession(Socket::Ptr sock, bool owner)
    : SocketStream(sock, owner) {
}

HttpRequest::Ptr HttpSession::recvRequest() {
    HttpRequestParser::Ptr parser(new HttpRequestParser);
    
    // 设置一个合理的缓冲区大小，例如 4KB
    int buff_size = 4096;
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr){
        delete[] ptr;
    });
    
    char* data = buffer.get();
    int offset = 0;     // 当前 buffer 中累积的有效数据总长度
    int last_len = 0;   // 上一次解析的长度 (用于 picohttpparser 避免重复扫描)
    int nparse = 0;     // parser 实际成功解析的头部字节数

    do {
        // 从 socket 中读取数据，追加到 buffer 后面
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        
        last_len = offset;
        offset += len;
        
        // 调用你设计的 parser，执行解析
        nparse = parser->execute(data, offset, last_len);
        
        if (parser->hasError() || nparse == -1) {
            // 报文格式错误
            close();
            return nullptr;
        }
        
        if (nparse > 0) {
            // 解析头部成功，nparse 即为 HTTP 请求头的总长度
            break;
        }
        
        if (nparse == -2) {
            // 报文头部还不完整，需要继续读取
            if (offset == buff_size) {
                // 如果 buffer 满了还没解析完，说明 HTTP 头部太长了，属于异常请求或恶意攻击
                close();
                return nullptr;
            }
        }
    } while (true);

    // 此时请求头已经解析完毕，拿到请求对象
    HttpRequest::Ptr req = parser->getData();
    
    // 解析 Content-Length，判断是否有 Body
    std::string length_str = req->getHeader("Content-Length", ""); // 假设你的 getHeader 第二个参数是默认值
    int64_t length = 0;
    if (!length_str.empty()) {
        try {
            length = std::stoll(length_str);
        } catch (...) {
            length = 0;
        }
    }

    // 如果有消息体 (Body)，则需要把 Body 读取完整
    if (length > 0) {
        std::string body;
        body.resize(length);

        // 计算在解析头部时，是否已经多读了一部分 body 数据进 buffer
        int body_read_len = offset - nparse;
        if (body_read_len > 0) {
            // 多读的部分直接 copy 进 body
            int copy_len = std::min((int64_t)body_read_len, length);
            memcpy(&body[0], data + nparse, copy_len);
        }

        // 计算还有多少 body 没有读完
        int64_t left_len = length - body_read_len;
        if (left_len > 0) {
            // 调用基类 SocketStream 的 readFixSize 阻塞读取剩余确切长度的 body
            if (readfixsize(&body[body_read_len], left_len) <= 0) {
                close();
                return nullptr;
            }
        }
        
        req->setBody(body);
    }

    // 如果你的 HttpRequest 有 init 等初始化方法（比如处理 keep-alive 标志），可以在这里调用
    // req->init();

    return req;
}

int HttpSession::sendResponse(HttpResponse::Ptr rsp) {
    // 将 HttpResponse 序列化为文本流
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    
    // 调用基类 SocketStream 的 writeFixSize 发送所有数据
    return writefixsize(data.c_str(), data.size());
}

} // namespace http
} // namespace DYX