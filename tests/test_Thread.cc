#include "../src/Thread.h"
#include "../src/Mutex.h"
#include "../src/Config.h"
#include "../src/Logdefine.h"
// DYX::Logger::Ptr g_logger = GET_ROOT();//获取根日志器

int g_cnt = 0;

DYX::RWMutex rw_mutex;//读写锁
using MutexType = DYX::RWMutex;
void func1(){
    STREAM_LOG_INFO(GET_ROOT()) << "name: " << DYX::Thread::getCurName()
                     << " this.name: " << DYX::Thread::getThis()->GetName()
                     << " id: " << DYX::GetThreadId()
                     << " this.id: " << DYX::Thread::getThis()->Gettid();
    for(int i = 0;i<1000000;++i){
        MutexType::WLock lock(rw_mutex);//写锁
        ++g_cnt;
    }
}

void func2(){
    while(true){
        STREAM_LOG_INFO(GET_ROOT())<<"************************************";
        // sleep(1);
    }
}

void func3(){
    while(true){
        STREAM_LOG_INFO(GET_ROOT())<<"====================================";
        // sleep(1);
    }
}

int main() {
    DYX::g_log_defines->definethis();//实例化，避免编译器优化掉
    // DYX::g_log_defines;    
    STREAM_LOG_DEBUG(GET_ROOT()) << "Thread test begin";
    YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/log.yaml");
    std::cout<<"============================================"<<std::endl;

    bool ret  = DYX::ConfigManager::LoadFromYaml(root);
    if(!ret){
        STREAM_LOG_ERROR(GET_ROOT()) << "LoadFromYaml failed";
        return -1;
    }

    std::cout<<"============================================"<<std::endl;
    std::vector<DYX::Thread::Ptr> thrds;
    // for(int i = 0;i<5;++i){
    //     DYX::Thread::Ptr thrd = std::make_shared<DYX::Thread>(func1,"thread_" + std::to_string(i));
    //     thrds.push_back(thrd);  
    // }
    for(int i = 0;i<1;++i){
        DYX::Thread::Ptr thrd = std::make_shared<DYX::Thread>(func2,"thread_" + std::to_string(i*2));
        thrds.push_back(thrd);  
        DYX::Thread::Ptr thrd2 = std::make_shared<DYX::Thread>(func3,"thread_" + std::to_string(i*2+1));
        thrds.push_back(thrd2);  
    }

    for(int i = 0;i<thrds.size();++i){
        thrds[i]->join();
    }   
    STREAM_LOG_DEBUG(GET_ROOT()) << "Thread test end";
    // STREAM_LOG_INFO(GET_ROOT()) << "g_cnt=" << g_cnt;
    // std::cout << DYX::SingleLoggerManager::GetInstance()->toYamlString() << std::endl;
    return 0;
}   