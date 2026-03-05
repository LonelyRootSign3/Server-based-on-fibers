#pragma once
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>
#include <string>
#include <vector>

namespace DYX{

// 二进制数组,提供基础类型的序列化,反序列化功能()
// 数据在网络中是以流的形式传输的（字节流）,所以需要支持序列化,反序列化功能
// 序列化:将数据结构或对象转换为字节流的过程
// 反序列化:将字节流转换为数据结构或对象的过程
class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> Ptr;
    struct Node{
        /**
         * @brief 构造指定大小的内存块
         * @param[in] s 内存块字节数
         */
        Node(size_t s);

        Node();

        /**
         * 析构函数,释放内存
         */
        ~Node();

        /// 内存块地址指针
        char* ptr;
        /// 下一个内存块地址
        Node* next;
        /// 内存块大小
        size_t size;
    };
    /**  
     * @brief 使用指定长度的内存块构造ByteArray
     * @param[in] base_size 内存块大小
     */
    ByteArray(size_t base_size = 4096);

    /**  
     * @brief 析构函数
     */
    ~ByteArray();

    /**
     * @brief 清空ByteArray，只保留最开始的内存块
     * @post m_position = 0, m_size = 0 
     */
    void clear();

    /**
     * @brief 写入size长度的数据
     * @param[in] buf 内存缓存指针
     * @param[in] size 数据大小
     * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
     */
    void write(const void* buf, size_t size);

    /**
     * @brief 读取size长度的数据
     * @param[out] buf 内存缓存指针
     * @param[in] size 数据大小
     * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
     * @exception 如果getReadSize() < size 则抛出 std::out_of_range
     */
    void read(void* buf, size_t size);

    /**
     * @brief 读取size长度的数据
     * @param[out] buf 内存缓存指针
     * @param[in] size 数据大小
     * @param[in] position 读取开始位置（不改变m_position）
     * @exception 如果 (m_size - position) < size 则抛出 std::out_of_range
     */
    void read(void* buf, size_t size, size_t position) const;

    //-----------------固定长度整数序列化接口-----------------
    //写
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);
    //读
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    // ---------变长整数接口（Varint）---------------------
    //写
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    //读
    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();
    

    //-----------------浮点数接口-----------
    void writeFloat(float value);
    void writeDouble(double value);
    float  readFloat();
    double readDouble();


    //-----------------字符串序列化接口-----------------
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);
    void writeStringVint(const std::string& value);
    void writeStringWithoutLength(const std::string& value);

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    //----------------------------位置控制接口---------------------------
    /**
     * @brief 返回ByteArray当前位置
     */
    size_t getPosition() const { return m_position;}

    /**
     * @brief 设置ByteArray当前位置
     * @post 如果m_position > m_size 则 m_size = m_position
     * @exception 如果m_position > m_capacity 则抛出 std::out_of_range
     */
    void setPosition(size_t v);


    //----------------------------------文件接口--------------------------

    /**
     * @brief 把ByteArray的数据写入到文件中
     * @param[in] name 文件名
     */
    bool writeToFile(const std::string& name) const;

    /**
     * @brief 从文件中读取数据
     * @param[in] name 文件名
     */
    bool readFromFile(const std::string& name);

    /**
     * @brief 返回内存块的大小
     */
    size_t getBaseSize() const { return m_base_node_size;}

    /**
     * @brief 返回可读取数据大小
     */
    size_t getReadSize() const { return m_size - m_position;}
   
    //----------------------------字节序列接口----------------------------
    /**
     * @brief 是否是小端
     */  
    bool isLittleEndian() const;

    /**
     * @brief 设置是否为小端
     */
    void setIsLittleEndian(bool val);
 
    /** 
     * @brief 将ByteArray里面的数据[m_position, m_size)转成std::string
     */
    std::string toString() const;

    /**
     * @brief 将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)
     */
    std::string toHexString() const;
  
    //------------------------------------iovec接口（减少系统调用次数）---------------------------------------

    /**
     * @brief 获取可读取的缓存,保存成iovec数组
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;

    /**
     * @brief 获取可读取的缓存,保存成iovec数组,从position位置开始（不改变m_position）
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @param[in] position 读取数据的位置
     * @return 返回实际数据的长度
     */ 
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief 获取可写入的缓存,保存成iovec数组
     * @param[out] buffers 保存可写入的内存的iovec数组
     * @param[in] len 写入的长度
     * @return 返回实际的长度
     * @post 如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    /**
     * @brief 返回数据的长度
     */
    size_t getSize() const { return m_size;}
private:
    /// 扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
    void addCapacity(size_t size);
    //获取可用的容量大小
    size_t getFreeCapacity() { return m_capacity - m_position;}
private:
    size_t m_base_node_size;/// 内存块的大小
    size_t m_position;/// 当前操作位置
    size_t m_capacity;/// 当前的总容量
    size_t m_size;/// 当前写入的有效数据的大小
    int8_t m_endian;// 字节序,默认大端
    Node* m_root;/// 第一个内存块指针
    Node* m_cur;// 当前操作的内存块指针
};




}
