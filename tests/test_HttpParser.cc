#include "../src/Http/HttpParser.h"
#include <iostream>
#include <string>
#include <vector>

using namespace DYX;
using namespace DYX::http;

void test_request_parser() {
    std::cout << "========== 测试 HttpRequestParser ==========" << std::endl;
    
    // 模拟从 Socket 接收到的 HTTP 请求报文
    std::string raw_request = 
        "GET /search?q=picohttpparser&lang=cpp HTTP/1.1\r\n"
        "Host: github.com\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\n"
        "Connection: keep-alive\r\n\r\n";

    // 因为 execute 接收 char*，我们将 string 放入 vector 中模拟 buffer
    std::vector<char> buffer(raw_request.begin(), raw_request.end());

    HttpRequestParser::Ptr parser = std::make_shared<HttpRequestParser>();
    
    // 执行解析
    int nparse = parser->execute(buffer.data(), buffer.size(), 0);

    if (parser->hasError()) {
        std::cout << "请求解析失败：格式错误！" << std::endl;
        return;
    }

    if (parser->isFinished()) {
        std::cout << "请求解析成功！共解析头部字节数: " << nparse << std::endl;
        HttpRequest::Ptr req = parser->getData();
        
        std::cout << "\n--- 解析后的 HttpRequest 对象 ---" << std::endl;
        std::cout << req->toString() << std::endl;
        
        // 验证参数解析 (你的框架里自带的 initQueryParam)
        std::cout << "--- 验证 Query 参数提取 ---" << std::endl;
        std::cout << "q = " << req->getParam("q") << std::endl;
        std::cout << "lang = " << req->getParam("lang") << std::endl;
    } else {
        std::cout << "请求解析未完成：数据不完整，需要继续 recv" << std::endl;
    }
    std::cout << std::endl;
}

void test_response_parser() {
    std::cout << "========== 测试 HttpResponseParser ==========" << std::endl;
    
    // 模拟从后端服务器接收到的 HTTP 响应报文 (包含 Body)
    std::string raw_response = 
        "HTTP/1.1 200 OK\r\n"
        "Server: DYX-Web-Server/1.0\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 17\r\n"
        "Connection: close\r\n\r\n"
        "{\"status\": \"ok\"}"; // 17 字节的 body

    std::vector<char> buffer(raw_response.begin(), raw_response.end());

    HttpResponseParser::Ptr parser = std::make_shared<HttpResponseParser>();

    int nparse = parser->execute(buffer.data(), buffer.size(), 0);

    if (parser->hasError()) {
        std::cout << "响应解析失败：格式错误！" << std::endl;
        return;
    }

    if (parser->isFinished()) {
        std::cout << "响应解析成功！共解析头部字节数: " << nparse << std::endl;
        HttpResponse::Ptr rsp = parser->getData();

        // 【关键点】：picohttpparser 只负责解析 HTTP 头部！
        // 解析完头部后，nparse 就是头部的总长度。剩下的部分就是 Body。
        if (buffer.size() > (size_t)nparse) {
            std::string body(buffer.data() + nparse, buffer.size() - nparse);
            rsp->setBody(body);
        }

        std::cout << "\n--- 解析后的 HttpResponse 对象 ---" << std::endl;
        std::cout << rsp->toString() << std::endl;
    } else {
        std::cout << "响应解析未完成：数据不完整，需要继续 recv" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    test_request_parser();
    test_response_parser();
    return 0;
}