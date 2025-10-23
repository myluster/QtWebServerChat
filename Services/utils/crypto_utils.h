#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>
#include <vector>

// 加密工具类
class CryptoUtils {
public:
    // 使用OpenSSL 3.0的新API实现SHA256哈希函数
    static std::string sha256(const std::string& str);
    
    // 生成随机盐值
    static std::string generateSalt(size_t length = 32);
    
    // 使用盐值的SHA256哈希
    static std::string saltedSha256(const std::string& str, const std::string& salt);
    
    // 生成安全令牌
    static std::string generateSecureToken();
};

// 兼容旧接口
std::string sha256(const std::string& str);

#endif // CRYPTO_UTILS_H