#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include "Socket.h"
#include "IOManager.h"
#include "Config.h"
#include "Nocopyable.h"
namespace DYX{

struct TcpServerConf{
    using Ptr = std::shared_ptr<TcpServerConf>;

    //基础网络与连接配置
    std::vector<std::string> address;//服务器需要绑定和监听的地址列表。它可以同时包含多个地址支持 IPv4、IPv6 以及 Unix Socket 地址
    int keepalive = 0;//长连接标志。通常用来控制 TCP 的 Keep-Alive 机制或应用层（如 HTTP）的长连接保持
    int timeout = 1000 * 60 *2;//网络读写的超时时间，单位是毫秒。这里的默认值 1000 * 2 * 60 表示 2 分钟。如果在这段时间内客户端没有数据交互，服务器可能会主动断开连接

    // 服务器身份与类型
    std::string id;//服务器实例的唯一标识符。当你的系统中有多个服务器实例时，用来区分它们。
    std::string type = "http";//当前服务器的应用层协议类型。常见的有 http（网页服务）、ws（WebSocket服务）
    std::string name;//服务器的名称。常用于日志打印，或者在 HTTP 响应报文的 Server 头中向客户端展示

    // SSL/TLS 安全配置
    int ssl = 0;//是否启用 SSL 加密（即 HTTPS/WSS）。通常 1 代表开启，0 代表关闭。
    std::string cert_file;//SSL 数字证书文件（公钥）的系统路径。
    std::string key_file;//SSL 私钥文件的系统路径。当 ssl 开启时，必须配置此项与 cert_file 来建立加密连接

    //多线程协程调度器配置
    std::string accept_worker;//专门负责监听和接收新连接（accept）的调度器名称
    std::string io_worker;//专门负责处理已建立连接的 Socket IO 读写事件的调度器名称。
    std::string business_worker;//专门负责处理具体业务逻辑的调度器名称。这种设计可以将耗时的业务逻辑与底层的网络 IO 分离开来，提高并发性能。
    //扩展参数
    std::map<std::string, std::string> args;//提供一定的扩展性，未来如果需要给服务器传递额外的自定义配置，就可以直接放进 args 里，而不需要改动 C++ 的结构体定义。

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const TcpServerConf& oth) const {
        return address == oth.address
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name
            && ssl == oth.ssl
            && cert_file == oth.cert_file
            && key_file == oth.key_file
            && accept_worker == oth.accept_worker
            && io_worker == oth.io_worker
            && business_worker == oth.business_worker
            && args == oth.args
            && id == oth.id
            && type == oth.type;
    }

};


/// @brief 从字符串解析 TCP 服务器配置
template<>
class LexicalCast<std::string, TcpServerConf>{
public:
    TcpServerConf operator()(const std::string &conf){
        YAML::Node node = YAML::Load(conf);
        TcpServerConf ret;
        if(node["address"].IsDefined())
            ret.address = node["address"].as<std::vector<std::string>>();
        ret.keepalive = node["keepalive"].as<int>();
        ret.timeout = node["timeout"].as<int>();
        ret.id = node["id"].as<std::string>();
        ret.type = node["type"].as<std::string>();
        ret.name = node["name"].as<std::string>();
        ret.ssl = node["ssl"].as<int>();
        ret.cert_file = node["cert_file"].as<std::string>();
        ret.key_file = node["key_file"].as<std::string>();
        ret.accept_worker = node["accept_worker"].as<std::string>();
        ret.io_worker = node["io_worker"].as<std::string>();
        ret.business_worker = node["business_worker"].as<std::string>();
        ret.args = LexicalCast<std::string, std::map<std::string, std::string>>()(node["args"].as<std::string>(""));
        return ret;
    }
};


template<>
class LexicalCast<TcpServerConf, std::string>{
public:
    std::string operator()(const TcpServerConf& conf) {
        YAML::Node node;
        node["id"] = conf.id;
        node["type"] = conf.type;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["ssl"] = conf.ssl;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        node["accept_worker"] = conf.accept_worker;
        node["io_worker"] = conf.io_worker;
        node["business_worker"] = conf.business_worker;
        node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>
            , std::string>()(conf.args));
        for(auto& i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

class TcpServer : public std::enable_shared_from_this<TcpServer> ,Nocopy {
public:
    using Ptr = std::shared_ptr<TcpServer>;
    TcpServer( IOManager *acceptWorker = IOManager::GetThis()
                , IOManager *ioWorker = IOManager::GetThis()
                , IOManager *businessWorker = IOManager::GetThis());
    virtual ~TcpServer();

    TcpServerConf::Ptr getConfig() const { return m_config;}
    std::vector<Socket::Ptr> getSocks() const { return m_socks;}
    bool isStop() const { return m_isStop ;}
    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }

    void setRecvTimeout( uint64_t v) { m_recvTimeout = v;}
    virtual void setName(const std::string &name) { m_name = name;}
    void setConfig(TcpServerConf::Ptr v) { m_config = v;}
    void setConfig(const TcpServerConf &v);


    virtual bool start();
    virtual void stop();
    /**
     * @brief 绑定地址
     * @return 返回是否绑定成功
     */
    virtual bool bind(Address::Ptr addr, bool ssl = false);

    /**
     * @brief 绑定地址数组
     * @param[in] addrs 需要绑定的地址数组
     * @param[out] fails 绑定失败的地址
     * @return 是否绑定成功
     */
    virtual bool bind(const std::vector<Address::Ptr>& addrs
                        ,std::vector<Address::Ptr>& fails
                        ,bool ssl = false);

    
    bool loadCertificates(const std::string& cert_file, const std::string& key_file);
protected:
    //处理新连接的socket类
    virtual void handleClient(Socket::Ptr client);
    //开始接受连接
    virtual void startAccept(Socket::Ptr sock);
protected:
TcpServerConf::Ptr m_config;//服务器配置

std::vector<Socket::Ptr> m_socks;//监听套接字数组(需要支持ipv4 ipv6 unix所以需要多个监听fd 来 bind 不同的 address)

IOManager *m_acceptWorker;//接受连接的调度器
IOManager *m_ioWorker;//处理IO事件的调度器
IOManager *m_businessWorker;//处理业务逻辑的调度器

uint64_t m_recvTimeout;//连接超时时间
std::string m_name;//服务器名称
std::string m_type;//服务器类型
bool m_isStop;//服务器运行状态
bool m_ssl;//ssl加密标志

};

}
