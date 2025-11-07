#include "../src/Config.h"
#include "../src/Logdefine.h"
// 类型转字符串
std::string nodeTypeToString(YAML::NodeType::value type) 
{
    switch(type) {
        case YAML::NodeType::Undefined: return "Undefined";
        case YAML::NodeType::Null:      return "Null";
        case YAML::NodeType::Scalar:    return "Scalar";
        case YAML::NodeType::Sequence:  return "Sequence";
        case YAML::NodeType::Map:       return "Map";
        default: return "Unknown";
    }
}

// 打印函数
void print_yaml(const YAML::Node& node, int level = 0) 
{
    std::string indent(level * 2, ' '); // 缩进：2个空格/层

    if(node.IsScalar()) {
        std::cout << indent 
                  << node.Scalar() 
                  << "   [" << nodeTypeToString(node.Type()) << "]"
                  << std::endl;
    } 
    else if(node.IsNull()) {
        std::cout << indent 
                  << "NULL"
                  << "   [" << nodeTypeToString(node.Type()) << "]"
                  << std::endl;
    } 
    else if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it) {
            std::cout << indent 
                      << it->first.as<std::string>() 
                      << ":   [" << nodeTypeToString(it->second.Type()) << "]"
                      << std::endl;
            print_yaml(it->second, level + 1);
        }
    } 
    else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            std::cout << indent 
                      << "-   [" << nodeTypeToString(node[i].Type()) << "]"
                      << std::endl;
            print_yaml(node[i], level + 1);
        }
    }
}

void TestYaml(){
    YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/log.yaml");// 加载配置文件
    // print_yaml(root , 0); // 打印配置文件内容
    // STREAM_LOG_INFO(GET_ROOT())<<root.Scalar();
    STREAM_LOG_INFO(GET_ROOT())<<root["root"].IsDefined();
    STREAM_LOG_INFO(GET_ROOT())<<root["loggers"].IsDefined();
    STREAM_LOG_INFO(GET_ROOT())<<root["logs"].IsDefined();
    STREAM_LOG_INFO(GET_ROOT())<<root["logs"][0]["name"];
    STREAM_LOG_INFO(GET_ROOT())<<root["logs"][1]["name"];
}

// DYX::ConfigVar<int>::Ptr g_int_valx_conf = DYX::ConfigManager::Lookup<int>("g_int_val", 100, "global int value");

//约定优先于配置（先约定,然后需要修改再读取配置文件）

#if 0
DYX::ConfigVar<int>::Ptr g_int_val_conf = DYX::ConfigManager::Lookup<int>("g_int_val", 10, "global int value");
DYX::ConfigVar<float>::Ptr g_float_val_conf = DYX::ConfigManager::Lookup<float>("g_float_val", 3.14, "global float value");
DYX::ConfigVar<std::vector<int>>::Ptr g_vec_val_conf = DYX::ConfigManager::Lookup<std::vector<int>>("g_vec_val", {1, 2, 3}, "global vector value"); 
DYX::ConfigVar<std::list<int>>::Ptr g_lst_val_conf = DYX::ConfigManager::Lookup<std::list<int>>("g_list_val", {1, 2, 3}, "global list value");
DYX::ConfigVar<std::set<int>>::Ptr  g_set_val_conf = DYX::ConfigManager::Lookup<std::set<int>>("g_set_val", {1, 2, 3}, "global set value");
DYX::ConfigVar<std::map<std::string, int>>::Ptr g_map_val_conf = 
                DYX::ConfigManager::Lookup<std::map<std::string, int>>("g_map_val", {{"a", 1}, {"b", 2}, {"c", 3}}, "global map value");
DYX::ConfigVar<std::unordered_set<int>>::Ptr g_uset_val_conf = 
                DYX::ConfigManager::Lookup<std::unordered_set<int>>("g_uset_val", {1, 2, 3}, "global unordered set value");
DYX::ConfigVar<std::unordered_map<std::string, int>>::Ptr g_umap_val_conf = 
                DYX::ConfigManager::Lookup<std::unordered_map<std::string, int>>("g_umap_val", {{"a", 1}, {"b", 2}, {"c", 3}}, "global unordered map value");

  
void TestConfig(){
    STREAM_LOG_INFO(GET_ROOT())<<" before "<<g_int_val_conf->getValue();// 获取配置变量的值
    STREAM_LOG_INFO(GET_ROOT())<<" before "<<g_int_val_conf->ToString();// 将配置变量的值转换为字符串

// 遍历配置变量的值
#define XX(g_var, name, prefix) \
    { \
        auto v = g_var->getValue(); \
        for(auto& i : v) { \
            STREAM_LOG_INFO(GET_ROOT()) << #prefix " " #name ": " << i; \
        } \
        STREAM_LOG_INFO(GET_ROOT()) << #prefix " " #name " yaml: " << g_var->ToString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto v = g_var->getValue(); \
        for(auto& i : v) { \
            STREAM_LOG_INFO(GET_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        STREAM_LOG_INFO(GET_ROOT()) << #prefix " " #name " yaml: " << g_var->ToString();\
    }

    XX(g_vec_val_conf, vec_val, before);
    XX(g_lst_val_conf, lst_val, before);
    XX(g_set_val_conf, set_val, before);
    XX(g_uset_val_conf, uset_val, before);
    XX_M(g_map_val_conf, map_val, before);
    XX_M(g_umap_val_conf, umap_val, before);

    YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/test.yaml");// 加载配置文件
    DYX::ConfigManager::LoadFromYaml(root); // 加载配置文件内容,修改配置变量的值
//测试配置是否被修改

    STREAM_LOG_INFO(GET_ROOT())<<" after "<<g_int_val_conf->getValue();// 获取配置变量的值
    STREAM_LOG_INFO(GET_ROOT())<<" after "<<g_int_val_conf->ToString();// 将配置变量的值转换为字符串

    XX(g_vec_val_conf, vec_val, after);
    XX(g_lst_val_conf, lst_val, after);
    XX(g_set_val_conf, set_val, after);
    XX(g_uset_val_conf, uset_val, after);
    XX_M(g_map_val_conf, map_val, after);
    XX_M(g_umap_val_conf, umap_val, after);
}

#endif

void test_log() {
    // (void)DYX::g_log_defines;
    DYX::g_log_defines->showchangeCallbacks();
    // DYX::SingleLoggerManager::GetInstance()->ShowLoggersName();
    // std::cout<<"初始配置的数量 : "<<DYX::g_log_defines->getValue().size()<<std::endl;
    STREAM_LOG_DEBUG(GET_ROOT())<<"没有加载配置文件前的日志器信息:";
    std::cout<<DYX::SingleLoggerManager::GetInstance()->toYamlString();//日志器信息
    std::cout<<"============================="<<std::endl;
    YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/log.yaml");//从文件加载配置
    bool ret = DYX::ConfigManager::LoadFromYaml(root);//加载配置文件内容,修改配置变量的值
    if(ret == false){
        STREAM_LOG_ERROR(GET_ROOT())<<"load log config failed";
        return;
    }
    std::cout << "=============" << std::endl;
    STREAM_LOG_DEBUG(GET_ROOT())<<"加载配置文件后的日志器信息:";
    std::cout << DYX::SingleLoggerManager::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << "Loaded log config: " << std::endl;
    std::cout << root << std::endl;
    // STREAM_LOG_INFO(system_log) << "hello system" << std::endl;
    STREAM_LOG_LEVEL(GET_ROOT(), DYX::Loglevel::FATAL) <<"测试";
    STREAM_LOG_INFO(GET_ROOT()) <<"测试";
    std::cout << "=============" << std::endl;

    std::cout << DYX::SingleLoggerManager::GetInstance()->toYamlString() << std::endl;
    STREAM_LOG_LEVEL(GET_ROOT(), DYX::Loglevel::FATAL) <<"测试";
    STREAM_LOG_INFO(GET_ROOT()) <<"测试";

    // std::cout<<GET_ROOT()->AppendSize()<<std::endl; 
}

int main() {
    // TestYaml();
    // TestConfig(); 
    // test_log();
    DYX::g_log_defines->definethis();
    YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/log.yaml");//从文件加载配置
    bool ret = DYX::ConfigManager::LoadFromYaml(root);//加载配置文件内容,修改配置变量的值
    if(ret == false){
        STREAM_LOG_ERROR(GET_ROOT())<<"load log config failed";
        return -1;
    }
    DYX::ConfigManager::Visit([](DYX::ConfigVarBase::Ptr var) {
        STREAM_LOG_INFO(GET_ROOT()) << "name= " << var->Get_name();
        STREAM_LOG_INFO(GET_ROOT())<< "description= " << var->Get_desc();
        STREAM_LOG_INFO(GET_ROOT()) << "type= " << var->GetTypeName();
        STREAM_LOG_INFO(GET_ROOT())<< "value= " << var->ToString();
    });
    return 0;
}
