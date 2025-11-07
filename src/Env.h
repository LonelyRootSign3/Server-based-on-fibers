#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "Singleton.h"
namespace DYX {

// 采用单例模式保证程序的环境变量是全局唯一的，便于统一管理。
// Env::init 利用 /proc/<pid>/exe 和命令行解析，\
把运行时上下文信息（程序所在目录、启动参数）集中保存在 Env 实例里，\
后续任何组件都能通过单例拿到统一的环境数据，避免重复解析或到处传参。
class Env {
public:
    //初始化方法
    bool init(int argc, char** argv);
    //环境变量相关
    void add(const std::string& key, const std::string& val);
    bool has(const std::string& key);
    void del(const std::string& key);
    std::string get(const std::string& key, const std::string& default_value = "");
    //帮助信息相关
    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();
    //路径相关
    const std::string& getExe() const { return _exe_path; }
    const std::string& getCwd() const { return _m_cwd; }
    //系统环境变量
    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_value = "");
    //路径转换
    std::string getAbsolutePath(const std::string& path) const;
    std::string getAbsoluteWorkPath(const std::string& path) const;
    std::string getConfigPath() const;



private:
    std::unordered_map<std::string, std::string> _m_args;//存储的自定义环境变量
    std::vector<std::pair<std::string, std::string>> _m_helps;//帮助信息

    std::string _program_name;//程序名称(来自argv[0])
    std::string _exe_path;//可执行程序路径
    std::string _m_cwd;//当前工作目录路径
};

using EnvMger = Singleton<Env>;// 声明一个Env的单例管理器

}