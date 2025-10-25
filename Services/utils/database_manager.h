#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include "crypto_utils.h"
#include "load_balancer.h"  // 包含负载均衡器头文件以支持分布式数据库连接

// 数据库实例信息结构体
// 用于存储数据库实例的连接信息和健康状态
struct DatabaseInstance {
    std::string host;       // 数据库主机地址
    int port;               // 数据库端口
    std::string user;       // 用户名
    std::string password;   // 密码
    std::string database;    // 数据库名称
    bool isHealthy;         // 健康状态标志
    
    // 构造函数初始化所有字段
    DatabaseInstance(const std::string& h, int p, const std::string& u, 
                     const std::string& pwd, const std::string& db)
        : host(h), port(p), user(u), password(pwd), database(db), isHealthy(true) {}
};

/**
 * @brief 数据库管理器类（单例模式）
 * 
 * 提供线程安全的MySQL数据库连接管理和操作功能，
 * 集成了负载均衡功能以支持分布式数据库部署。
 * 
 * 主要特性：
 * 1. 单例模式确保全局唯一实例
 * 2. 线程安全的操作（使用互斥锁保护）
 * 3. 自动负载均衡的数据库连接选择
 * 4. 健康检查和故障转移机制
 * 5. 密码加密存储（SHA256）
 * 6. SQL注入防护（使用预处理语句）
 */
class DatabaseManager {
public:
    /**
     * @brief 获取DatabaseManager单例实例
     * @return DatabaseManager实例的引用
     */
    static DatabaseManager& getInstance();
    
    /**
     * @brief 添加数据库实例到负载均衡器
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param user 用户名
     * @param password 密码
     * @param database 数据库名称
     * @param weight 负载均衡权重（默认为1）
     */
    void addDatabaseInstance(const std::string& host, int port, 
                            const std::string& user, const std::string& password, 
                            const std::string& database, int weight = 1);
    
    /**
     * @brief 更新数据库实例的健康状态
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param isHealthy 健康状态
     */
    void updateInstanceHealth(const std::string& host, int port, bool isHealthy);
    
    /**
     * @brief 连接到下一个健康的数据库实例
     * 使用负载均衡器选择最优的数据库实例进行连接
     * @return 连接成功返回true，否则返回false
     */
    bool connect();
    
    /**
     * @brief 断开当前数据库连接
     */
    void disconnect();
    
    /**
     * @brief 检查当前数据库连接状态
     * @return 连接有效返回true，否则返回false
     */
    bool isConnected() const;

    /**
     * @brief 私有的、无锁的连接实现
     * 假设调用者已经持有了 mutex_
     */
    bool connect_impl();
    
    /**
     * @brief 私有的、无锁的连接状态检查实现
     * 假设调用者已经持有了 mutex_
     */
    bool isConnected_impl() const;
    
    /**
     * @brief 获取原始数据库连接（用于直接执行查询）
     * @return MYSQL连接指针的void指针形式
     */
    void* getConnection() const { return static_cast<void*>(connection_); }
    
    /**
     * @brief 创建新用户
     * @param username 用户名
     * @param password 密码（明文，将被哈希存储）
     * @param email 邮箱地址
     * @param userId 输出参数，成功时返回新用户的ID
     * @return 成功返回true，否则返回false
     */
    bool createUser(const std::string& username, const std::string& password, const std::string& email, int& userId); 
    
    /**
     * @brief 根据用户名获取用户信息
     * @param username 用户名
     * @param userId 输出参数，成功时返回用户ID
     * @param passwordHash 输出参数，成功时返回密码哈希值
     * @return 成功返回true，否则返回false
     */
    bool getUserByUsername(const std::string& username, int& userId, std::string& passwordHash);
    
    /**
     * @brief 检查用户名是否存在
     * @param username 用户名
     * @return 存在返回true，否则返回false
     */
    bool userExists(const std::string& username);
    
    /**
     * @brief 获取当前连接的数据库主机地址
     * @return 当前主机地址字符串
     */
    std::string getHost() const { return currentHost_; }
    
    /**
     * @brief 获取当前连接的数据库用户名
     * @return 当前用户名字符串
     */
    std::string getUser() const { return currentUser_; }
    
    /**
     * @brief 获取当前连接的数据库名称
     * @return 当前数据库名称字符串
     */
    std::string getName() const { return currentDatabase_; }
    
    /**
     * @brief 获取当前连接的数据库端口
     * @return 当前端口号
     */
    int getPort() const { return currentPort_; }
    
    /**
     * @brief 获取互斥锁（用于线程安全）
     * @return 互斥锁引用
     */
    std::mutex& mutex() const { return mutex_; }

private:
    /**
     * @brief 私有构造函数（单例模式要求）
     */
    DatabaseManager();
    
    /**
     * @brief 私有析构函数（单例模式要求）
     */
    ~DatabaseManager();
    
    /**
     * @brief 私有的无锁版本，供内部调用避免递归锁死
     * @param username 用户名
     * @return 存在返回true，否则返回false
     */
    bool userExistsNoLock(const std::string& username);
    
    MYSQL* connection_;         // MySQL连接指针
    bool connected_;            // 连接状态标志
    mutable std::mutex mutex_;  // 用于线程安全的互斥锁
    
    // 当前连接的数据库信息
    std::string currentHost_;       // 当前主机地址
    std::string currentUser_;       // 当前用户名
    std::string currentPassword_;   // 当前密码
    std::string currentDatabase_;   // 当前数据库名称
    int currentPort_;               // 当前端口号
    
    // 负载均衡器引用
    LoadBalancer& loadBalancer_;
    
    // 服务名称（用于负载均衡器标识）
    static const std::string SERVICE_NAME;
};

#endif // DATABASE_MANAGER_H