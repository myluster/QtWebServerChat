#ifndef REDIS_MANAGER_H
#define REDIS_MANAGER_H

#include <hiredis/hiredis.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "logger.h"

/**
 * @brief Redis管理器类（单例模式）
 * 
 * 提供线程安全的Redis连接管理和操作功能，
 * 支持连接池、发布/订阅、哈希操作等。
 * 
 * 主要特性：
 * 1. 单例模式确保全局唯一实例
 * 2. 线程安全的操作（使用互斥锁保护）
 * 3. 连接池管理
 * 4. 发布/订阅功能
 * 5. 哈希操作（HSET/HGET/HDEL等）
 * 6. 字符串操作（SET/GET/INCR等）
 * 7. 有序集合操作（ZADD/ZRANGE等）
 */
class RedisManager {
public:
    /**
     * @brief 获取RedisManager单例实例
     * @return RedisManager实例的引用
     */
    static RedisManager& getInstance();
    
    /**
     * @brief 初始化Redis连接
     * @param host Redis服务器主机地址
     * @param port Redis服务器端口
     * @param poolSize 连接池大小
     * @return 初始化成功返回true，否则返回false
     */
    bool initialize(const std::string& host, int port, int poolSize = 10);
    
    /**
     * @brief 断开所有Redis连接
     */
    void disconnect();
    
    /**
     * @brief 检查Redis连接状态
     * @return 连接有效返回true，否则返回false
     */
    bool isConnected() const;
    
    /**
     * @brief 获取互斥锁（用于线程安全）
     * @return 互斥锁引用
     */
    std::mutex& mutex() const { return mutex_; }
    
    // ==================== 字符串操作 ====================
    /**
     * @brief 设置字符串键值
     * @param key 键
     * @param value 值
     * @return 操作成功返回true，否则返回false
     */
    bool set(const std::string& key, const std::string& value);
    
    /**
     * @brief 获取字符串值
     * @param key 键
     * @param value 输出参数，成功时返回值
     * @return 操作成功返回true，否则返回false
     */
    bool get(const std::string& key, std::string& value);
    
    /**
     * @brief 原子递增操作
     * @param key 键
     * @param result 输出参数，成功时返回递增后的值
     * @return 操作成功返回true，否则返回false
     */
    bool incr(const std::string& key, long long& result);
    
    /**
     * @brief 删除一个或多个键
     * @param key 键
     * @return 操作成功返回true，否则返回false
     */
    bool del(const std::string& key);
    
    // ==================== 哈希操作 ====================
    /**
     * @brief 设置哈希字段值
     * @param key 哈希键
     * @param field 字段名
     * @param value 值
     * @return 操作成功返回true，否则返回false
     */
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    /**
     * @brief 获取哈希字段值
     * @param key 哈希键
     * @param field 字段名
     * @param value 输出参数，成功时返回值
     * @return 操作成功返回true，否则返回false
     */
    bool hget(const std::string& key, const std::string& field, std::string& value);
    
    /**
     * @brief 删除哈希字段
     * @param key 哈希键
     * @param field 字段名
     * @return 操作成功返回true，否则返回false
     */
    bool hdel(const std::string& key, const std::string& field);
    
    /**
     * @brief 获取哈希所有字段和值
     * @param key 哈希键
     * @param result 输出参数，成功时返回字段值映射
     * @return 操作成功返回true，否则返回false
     */
    bool hgetall(const std::string& key, std::unordered_map<std::string, std::string>& result);
    
    // ==================== 有序集合操作 ====================
    /**
     * @brief 向有序集合添加成员
     * @param key 有序集键
     * @param score 分数
     * @param member 成员
     * @return 操作成功返回true，否则返回false
     */
    bool zadd(const std::string& key, double score, const std::string& member);
    
    /**
     * @brief 获取有序集合指定范围的成员
     * @param key 有序集键
     * @param start 起始位置
     * @param stop 结束位置
     * @param result 输出参数，成功时返回成员列表
     * @return 操作成功返回true，否则返回false
     */
    bool zrange(const std::string& key, int start, int stop, std::vector<std::string>& result);
    
    // ==================== 发布/订阅操作 ====================
    /**
     * @brief 发布消息到频道
     * @param channel 频道名
     * @param message 消息内容
     * @return 操作成功返回true，否则返回false
     */
    bool publish(const std::string& channel, const std::string& message);
    
    /**
     * @brief 订阅频道（需要在单独线程中调用）
     * @param channels 频道列表
     * @param messageCallback 消息回调函数
     * @return 操作成功返回true，否则返回false
     */
    bool subscribe(const std::vector<std::string>& channels, 
                   std::function<void(const std::string&, const std::string&)> messageCallback);

private:
    /**
     * @brief 私有构造函数（单例模式要求）
     */
    RedisManager();
    
    /**
     * @brief 私有析构函数（单例模式要求）
     */
    ~RedisManager();
    
    /**
     * @brief 创建Redis连接
     * @param host Redis服务器主机地址
     * @param port Redis服务器端口
     * @return Redis连接上下文指针
     */
    redisContext* createConnection(const std::string& host, int port);
    
    /**
     * @brief 从连接池获取连接
     * @return Redis连接上下文指针
     */
    redisContext* getConnection();
    
    /**
     * @brief 将连接归还到连接池
     * @param ctx Redis连接上下文指针
     */
    void returnConnection(redisContext* ctx);
    
    /**
     * @brief 检查连接是否有效
     * @param ctx Redis连接上下文指针
     * @return 连接有效返回true，否则返回false
     */
    bool isConnectionValid(redisContext* ctx) const;
    
    // 连接池相关
    std::vector<redisContext*> connectionPool_;
    mutable std::mutex poolMutex_;
    std::string host_;
    int port_;
    int poolSize_;
    bool initialized_;
    mutable std::mutex mutex_;
};

#endif // REDIS_MANAGER_H