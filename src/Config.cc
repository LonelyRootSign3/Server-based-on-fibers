#include "Config.h"
namespace DYX{

static DYX::Logger::Ptr g_logger = GET_ROOT();//系统日志


//获取配置项的基类指针，当你不知道具体类型时，可以用基类来做统一处理。
ConfigVarBase::Ptr ConfigManager::LookupBase(const std::string &name)
{
    MutexType::RLock lock(getMutex());
    auto it =  getDatas().find(name);
    if(it != getDatas().end()) {
        return it->second;
    }
    // STREAM_LOG_ERROR(GET_ROOT()) << "Unknown config: " << name;
    return nullptr;
}

//递归地遍历 YAML 节点，并把每个配置项“扁平化”为 key-value 形式存入列表。
static void ListAllConfigs(const std::string &prefix,//前缀，用于打印配置项的层级关系
                          const YAML::Node &node,
                        std::list<std::pair<std::string,YAML::Node>> *out)
{
        if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
            != std::string::npos) {
            STREAM_LOG_ERROR(g_logger) << "Invalid prefix: " << prefix;
            return;
        }
        out->push_back(std::make_pair(prefix, node));
        if(node.IsMap()) {
            for(auto it = node.begin(); it != node.end(); ++it) {
                ListAllConfigs(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), 
                            it->second, out);  
            }
        }
}

// 使用YAML::Node初始化配置模块
bool ConfigManager::LoadFromYaml(const YAML::Node &node)
{
    std::list<std::pair<std::string,YAML::Node>> all_configs;
    ListAllConfigs("", node, &all_configs);
    // STREAM_LOG_INFO(GET_ROOT())<<"all_configs size="<<all_configs.size();   
    for(auto &p : all_configs) {
        std::string key = p.first;
        if(key.empty()) continue;

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);//转为小写
        auto var = LookupBase(key);
        if(var == nullptr){
            STREAM_LOG_ERROR(GET_ROOT()) << "Unknown config: " << key;
            continue;
        }
        // STREAM_LOG_DEBUG(GET_ROOT()) << "LoadConfig:\nkey: " << key
        //                             << "\nvalue : \n" << p.second;
        if(p.second.IsScalar()){//如果是标量
            // STREAM_LOG_DEBUG(GET_ROOT())<<"标量类型";
            if(!var->Fromstring(p.second.Scalar())){
                STREAM_LOG_ERROR(GET_ROOT()) << "ConfigVar::Fromstring failed: " << key
                                            << " value: " << p.second.Scalar();
                return false;
            }
        }else{//如果是复杂类型
            // STREAM_LOG_DEBUG(GET_ROOT())<<"非标量类型";
            std::stringstream ss;
            ss << p.second;
            if(!var->Fromstring(ss.str())){
                STREAM_LOG_ERROR(GET_ROOT()) << "ConfigVar::Fromstring failed: \nkey : " << key
                                            << "\nvalue: \n" << ss.str();
                return false;
            }
        }
        // STREAM_LOG_DEBUG(GET_ROOT()) << "LoadConfigFile: " << key
        //                             << " value: " << var->ToString();
    }
    return true;
}

// 加载指定文件夹（path）里面的配置文件
void ConfigManager::LoadFromConfigDir(const std::string &path , bool force)
{

}

//遍历配置模块里面所有配置项, 打印所有配置项的值
void ConfigManager::Visit(std::function<void(ConfigVarBase::Ptr)> cb)
{
    MutexType::RLock lock(getMutex());
    auto ConfMap = getDatas();//获取配置项集合（智能指针的拷贝）
    for(auto &p : ConfMap) {
        cb(p.second);
    }
}

ConfigManager::ConfigMap& ConfigManager::getDatas(){//确保配置项集合先于其他静态对象创建
    static ConfigMap datas;//静态空间
    return datas;
}
ConfigManager::MutexType& ConfigManager::getMutex(){//确保读写锁先于其他静态对象创建(属于类的私有静态成员函数方法)
    static MutexType s_mutex;
    return s_mutex;
}

}
