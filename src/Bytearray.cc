#include "Bytearray.h"
#include "Endian.h"
#include "Log.h"
#include <cstring>
#include <fstream>
#include <math.h>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
namespace DYX{

static Logger::Ptr g_logger = GET_NAME_LOGGER("system");

ByteArray::Node::Node()
    :size(0)
    ,next(nullptr)
    ,ptr(nullptr){}

ByteArray::Node::Node(size_t s)  
    :size(s)
    ,next(nullptr){
        ptr = new char[size];
}
ByteArray::Node::~Node(){
    if(ptr)
        delete[] ptr;
}

ByteArray::ByteArray(size_t base_size)
    :m_base_node_size(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(MY_BIG_ENDIAN){
    m_root = new Node(m_base_node_size) ;
    m_cur = m_root;
} 

ByteArray::~ByteArray(){
    clear();
    delete m_root;
}

void ByteArray::clear(){
    m_position = m_size = 0;
    m_capacity = m_base_node_size;
    m_cur = m_root;
    Node* tmp = m_root->next;
    while(tmp){
        m_cur->next = tmp->next;
        delete tmp;
        tmp = m_cur->next;
    }
    m_root->next = nullptr;
}
void ByteArray::addCapacity(size_t size){
    if(size == 0) return;
    size_t free = getFreeCapacity();
    if(free >= size) return;

    // 扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
    size -= free;
    int cnt = ceil(size*1.0/m_base_node_size);//需要添加的节点数(向上取整)
    Node *tmp = m_root;
    while(tmp->next){
        tmp = tmp->next;
    }
    
    Node *first = nullptr;
    for(int i = 0;i< cnt;i++){
        tmp->next = new Node(m_base_node_size);
        tmp = tmp->next;
        if(first == nullptr) first = tmp;
        m_capacity += m_base_node_size;
    }

    if(free == 0) m_cur = first;//如果之前没有空闲空间,则将当前指针指向第一个新节点


}

void ByteArray::write(const void* buf, size_t size){
    if(size == 0 || !buf) return;
    addCapacity(size);//懒加载,只有当写入数据时才扩容
    size_t offset = m_position % m_cur->size;//当前位置在当前节点中的偏移量(第几个字节)
    size_t leftcap = m_cur->size - offset;//当前节点剩余容量
    size_t src_offset = 0;//源数据的偏移量
    while(size){
        if(leftcap >= size){
            memcpy(m_cur->ptr + offset, (const char *)buf + src_offset,size);
            if(size == leftcap)
                m_cur = m_cur->next;
            m_position += size;
            break;
        }
        else{
            memcpy(m_cur->ptr + offset, (const char *)buf + src_offset,leftcap);
            m_position += leftcap;
            src_offset += leftcap;
            size -= leftcap;
            offset = 0;
            m_cur = m_cur->next;
            leftcap = m_cur->size;
        }
    }
    if(m_size < m_position) m_size = m_position;    

}

void ByteArray::read(void* buf, size_t size){
    if(size > getReadSize()){
        throw std::out_of_range(" read size out of range");
    }
    size_t offset = m_position % m_cur->size;//当前位置在当前节点中的偏移量(第几个字节)
    size_t leftcap = m_cur->size - offset;//当前节点剩余容量
    size_t dest_offset = 0;//读缓冲区的偏移量
    while(size){
        if(leftcap >= size){
            memcpy((char *)buf + dest_offset,m_cur->ptr + offset,size);
            if(size == leftcap)
                m_cur = m_cur->next;
            m_position += size;
            break;
        }
        else{
            memcpy((char *)buf + dest_offset,m_cur->ptr + offset, leftcap);
            m_position += leftcap;
            dest_offset += leftcap;
            size -= leftcap;
            offset = 0;
            m_cur = m_cur->next;
            leftcap = m_cur->size;
        }
    }
}
void ByteArray::read(void* buf, size_t size, size_t position) const{
    if(size > m_size - position){
        throw std::out_of_range("read size out of range");
    }
    int cnt1 = ceil(position * 1.0/m_base_node_size);
    int cnt2 = ceil(m_position * 1.0/m_base_node_size);
    Node *tmp;
    if(cnt1 == cnt2) tmp = m_cur;
    else{
        tmp = m_root;
        while(cnt1--) tmp = tmp->next;
    }
    size_t read_offset = position % tmp->size;//读取位置在当前节点中的偏移量(第几个字节)
    size_t leftcap = tmp->size - read_offset;
    size_t dest_offset = 0;
    while(size){
        if(leftcap >= size){
            memcpy((char *)buf + dest_offset,tmp->ptr + read_offset,size);
            if(size == leftcap)
                tmp = tmp->next;
            position += size;
            break;
        }
        else{
            memcpy((char *)buf + dest_offset,tmp->ptr + read_offset, leftcap);
            position += leftcap;
            dest_offset += leftcap;
            size -= leftcap;
            read_offset = 0;
            tmp = tmp->next;
            leftcap = tmp->size;
        }
    }

}

void ByteArray::setPosition(size_t v){
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

#define WRITE_FIXED(value) do{\
    if(m_endian != MY_BYTE_ORDER) {\
        value = byteswap(value);\
    }\
    ByteArray::write(&value, sizeof(value));\
} while(0)

#define READ_FIXED(type) do{\
    type v;\
    ByteArray::read(&v, sizeof(v));\
    if(m_endian != MY_BYTE_ORDER) {\
        v = byteswap(v);\
    }\
    return v;\
} while(0)

// 编码 Zigzag 格式
static uint32_t EncodeZigzag32( int32_t v){
    if(v < 0) return (uint32_t)(-v) * 2 - 1;
    return v * 2;
}

static uint64_t EncodeZigzag64( int64_t v){
    if(v < 0) return (uint64_t)(-v) * 2 - 1;
    return v * 2;
}

// 解码 Zigzag 格式
static int32_t DecodeZigzag32(uint32_t v){
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(uint64_t v){
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeFint8(int8_t value){ write(&value, sizeof(int8_t));}
void ByteArray::writeFuint8(uint8_t value){ write(&value, sizeof(uint8_t));}
void ByteArray::writeFint16(int16_t value){ WRITE_FIXED(value);}
void ByteArray::writeFuint16(uint16_t value){ WRITE_FIXED(value);}
void ByteArray::writeFint32(int32_t value){ WRITE_FIXED(value);}
void ByteArray::writeFuint32(uint32_t value){ WRITE_FIXED(value);}
void ByteArray::writeFint64(int64_t value){ WRITE_FIXED(value);}
void ByteArray::writeFuint64(uint64_t value){ WRITE_FIXED(value);}
  
int8_t   ByteArray::readFint8()  { int8_t v ; ByteArray::read(&v, sizeof(v)); return v; }
uint8_t  ByteArray::readFuint8() { uint8_t v; ByteArray::read(&v, sizeof(v)); return v; }
int16_t  ByteArray::readFint16() { READ_FIXED(int16_t);}
uint16_t ByteArray::readFuint16(){ READ_FIXED(uint16_t);}
int32_t  ByteArray::readFint32() { READ_FIXED(int32_t);}
uint32_t ByteArray::readFuint32(){ READ_FIXED(uint32_t);}
int64_t  ByteArray::readFint64() { READ_FIXED(int64_t);}
uint64_t ByteArray::readFuint64(){ READ_FIXED(uint64_t);}

void ByteArray::writeUint32(uint32_t value){
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80){
        tmp[i++] =  (value & 0x7f) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);  
}

void ByteArray::writeUint64(uint64_t value){
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80){
        tmp[i++] =  (value & 0x7f) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}


void ByteArray::writeInt32(int32_t value){ 
    writeUint32(EncodeZigzag32(value));
}


void ByteArray::writeInt64(int64_t value){ 
    writeUint64(EncodeZigzag64(value));
}

uint32_t ByteArray::readUint32(){
    uint32_t result = 0;
    for(int i = 0;i< 32;i+=7){
        uint8_t n = readFuint8();
        if(n < 0x80){
            result |= ((uint32_t)n) << i;
            break;
        } else {
            result |= (((uint32_t)(n & 0x7f)) << i);
        }
    }
    return result;
}

uint64_t ByteArray::readUint64(){
    uint64_t result = 0;
    for(int i = 0;i< 64;i+=7){
        uint8_t n = readFuint8();
        if(n < 0x80){
            result |= ((uint64_t)n) << i;
            break;
        } else {
            result |= (((uint64_t)(n & 0x7f)) << i);
        }
    }
    return result;
}

int32_t  ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}
int64_t  ByteArray::readInt64(){
    return DecodeZigzag64(readUint64());
}


void ByteArray::writeFloat(float value){
    uint32_t bf;
    memcpy(&bf,&value,sizeof(value));
    writeFint32(bf);
}
void ByteArray::writeDouble(double value){
    uint64_t bf;
    memcpy(&bf,&value,sizeof(value));
    writeFint64(bf);
}

float  ByteArray::readFloat(){
    uint32_t bf = readFuint32();
    float value;
    memcpy(&value,&bf,sizeof(bf));
    return value;
}
double ByteArray::readDouble(){
    uint64_t bf = readFuint64();
    double value;
    memcpy(&value,&bf,sizeof(bf));
    return value;
}

void ByteArray::writeStringF16(const std::string& value){
    writeFuint16(value.size());
    write(value.data(), value.size());
}
void ByteArray::writeStringF32(const std::string& value){
    writeFuint32(value.size());
    write(value.data(), value.size());
}
void ByteArray::writeStringF64(const std::string& value){
    writeFuint64(value.size());
    write(value.data(), value.size());
}
void ByteArray::writeStringVint(const std::string& value){
    writeUint64(value.size());
    write(value.data(), value.size());
}
void ByteArray::writeStringWithoutLength(const std::string& value){
    write(value.data(), value.size());
}

std::string ByteArray::readStringF16(){
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringF32(){
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringF64(){
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringVint(){
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}


bool ByteArray::writeToFile(const std::string& name)const {
    std::ofstream ofs; 
    ofs.open(name,std::ios::binary | std::ios::trunc);
    if(!ofs){
        STREAM_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    Node *buf = m_cur;
    int64_t read_size = getReadSize();
    int64_t offset = m_position % buf->size;//当前结点的偏移
    while(read_size > 0){ 
        int64_t len = read_size > (buf->size - offset) ? (buf->size - offset) : read_size;
        ofs.write((const char*)buf->ptr + offset, len);
        read_size -= len;
        offset = 0;
        buf = buf->next;
    }
    return true;
}
bool ByteArray::readFromFile(const std::string& name){
    std::ifstream ifs; 
    ifs.open(name,std::ios::binary);
    if(!ifs){
        STREAM_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buf(new char[m_base_node_size], [](char* ptr) { delete[] ptr;});//缓冲区
    while(!ifs.eof()) {
        ifs.read(buf.get(), m_base_node_size);
        write(buf.get(), ifs.gcount());//将缓冲区内容写入到ByteArray中
    }
    return true;
}

bool ByteArray::isLittleEndian () const{
    return m_endian == MY_LITTLE_ENDIAN;
}
void ByteArray::setIsLittleEndian(bool val){
    m_endian = val ? MY_LITTLE_ENDIAN : MY_BIG_ENDIAN;  
}

std::string ByteArray::toString() const{
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) return str;
    read(&str[0], getReadSize(),m_position);
    return str;
}

std::string ByteArray::toHexString() const{
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buf, uint64_t len) const{
    len = std::min(len, getReadSize());
    if(len == 0) return 0;
    uint64_t size = len;//用于返回
    size_t offset = m_position % m_base_node_size;//当前结点的偏移
    size_t leftcap = m_cur->size - offset;//当前结点的剩余容量

    struct iovec iov;
    Node *tmp = m_cur;
    while(len){
        if(leftcap >= len){
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = len;
            buf.push_back(iov);
            break;
        }else{
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = leftcap;
            len -= leftcap;
            offset = 0;
            tmp = tmp->next;
            leftcap = tmp->size;
            buf.push_back(iov);
        }
    }
    return size;
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buf, uint64_t len, uint64_t position) const {
    uint64_t read_size = m_size - position;
    len =  std::min(read_size,len);
    if(len == 0) return 0;
    uint64_t size = len;//用于返回

    int cnt1 = ceil(position * 1.0/m_base_node_size);
    int cnt2 = ceil(m_position * 1.0/m_base_node_size);
    Node *tmp;
    if(cnt1 == cnt2) tmp = m_cur;
    else{
        tmp = m_root;
        while(cnt1--) tmp = tmp->next;
    }
    size_t offset = position % m_base_node_size;//读取位置在当前节点中的偏移量(第几个字节)
    size_t leftcap = tmp->size - offset;
    struct iovec iov;
    while(len){
        if(leftcap >= len){
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = len;
            buf.push_back(iov);
            break;
        }else{
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = leftcap;
            len -= leftcap;
            offset = 0;
            tmp = tmp->next;
            leftcap = tmp->size;
            buf.push_back(iov);
        }
    }
    return size;

}
uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buf, uint64_t len){
    if(len == 0) return 0;
    addCapacity(len);//懒加载,只有当写入数据时才扩容
    uint64_t size = len;//用于返回
    size_t offset = m_position % m_base_node_size;//当前结点的偏移
    size_t leftcap = m_cur->size - offset;//当前结点的剩余容量
    struct iovec iov;
    Node *tmp = m_cur;
    while(len){
        if(leftcap >= len){
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = len;
            buf.push_back(iov);
            break;
        }else{
            iov.iov_base = tmp->ptr + offset;
            iov.iov_len = leftcap;
            len -= leftcap;
            offset = 0;
            tmp = tmp->next;
            leftcap = tmp->size;
            buf.push_back(iov);
        }
    }
    return size;
}

}

