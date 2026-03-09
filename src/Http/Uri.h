#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include <../Address.h>
namespace DYX{
/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/
class Uri {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Uri> Ptr;

    /**
     * @brief 解析字符串并创建 Uri 对象
     * @param uristr 完整的 URL 字符串 (例如 "http://www.baidu.com:80/search?q=c++")
     * @return 解析成功返回 Uri 智能指针，失败（格式错误）返回 nullptr
     */
    static Uri::Ptr Create(const std::string& uristr);

    /**
     * @brief 默认构造函数
     */
    Uri();

    // ================= 属性获取 (Getters) =================
    const std::string& getScheme() const { return m_scheme;}
    const std::string& getUserinfo() const { return m_userinfo;}
    const std::string& getHost() const { return m_host;}
    
    /**
     * @brief 获取路径。如果路径为空，默认返回 "/"
     */
    const std::string& getPath() const;
    const std::string& getQuery() const { return m_query;}
    const std::string& getFragment() const { return m_fragment;}
    
    /**
     * @brief 获取端口。如果未显式指定，会根据 scheme 自动推导 (http->80, https->443)
     */
    int32_t getPort() const;

    // ================= 属性设置 (Setters) =================
    void setScheme(const std::string& v) { m_scheme = v;}
    void setUserinfo(const std::string& v) { m_userinfo = v;}
    void setHost(const std::string& v) { m_host = v;}
    void setPath(const std::string& v) { m_path = v;}
    void setQuery(const std::string& v) { m_query = v;}
    void setFragment(const std::string& v) { m_fragment = v;}
    void setPort(int32_t v) { m_port = v;}

    // ================= 辅助方法 =================
    /**
     * @brief 将当前的 Uri 对象重新序列化为标准的 URL 字符串并输出到流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成 URL 字符串
     */
    std::string toString() const;

    /**
     * @brief 根据解析出的 Host 和 Port，直接生成项目底层的 Address 对象
     * @return 成功返回 Address 的智能指针，解析失败（如域名不存在）返回 nullptr
     */
    Address::Ptr createAddress() const;

private:
    /**
     * @brief 判断当前端口是否是该协议的默认端口
     */
    bool isDefaultPort() const;

private:
    std::string m_scheme;     ///< 协议 (例如 http, https, ws)
    std::string m_userinfo;   ///< 用户信息 (例如 username:password)
    std::string m_host;       ///< 主机名或 IP (例如 www.baidu.com)
    std::string m_path;       ///< 资源路径 (例如 /index.html)
    std::string m_query;      ///< 查询参数 (例如 ?id=1)
    std::string m_fragment;   ///< 片段标识 (例如 #section1)
    int32_t m_port;           ///< 端口号
};

}