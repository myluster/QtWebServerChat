#include "crypto_utils.h"
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>

// 使用OpenSSL 3.0的新API实现SHA256哈希函数
std::string CryptoUtils::sha256(const std::string& str) {
    // 使用EVP API (OpenSSL 3.0推荐方式)
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "";
    }
    
    const EVP_MD* md = EVP_sha256();
    if (!md) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (EVP_DigestUpdate(mdctx, str.c_str(), str.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    EVP_MD_CTX_free(mdctx);
    
    // 转换为十六进制字符串
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

// 生成随机盐值
std::string CryptoUtils::generateSalt(size_t length) {
    std::vector<unsigned char> salt(length);
    if (RAND_bytes(salt.data(), length) != 1) {
        // 如果RAND_bytes失败，使用替代方法
        std::cerr << "Warning: RAND_bytes failed, using alternative method for salt generation" << std::endl;
        for (size_t i = 0; i < length; i++) {
            salt[i] = static_cast<unsigned char>(rand() % 256);
        }
    }
    
    // 转换为十六进制字符串
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
    }
    
    return ss.str();
}

// 使用盐值的SHA256哈希
std::string CryptoUtils::saltedSha256(const std::string& str, const std::string& salt) {
    return sha256(str + salt);
}

// 生成安全令牌
std::string CryptoUtils::generateSecureToken() {
    // 生成随机数据
    std::vector<unsigned char> random_data(32);
    if (RAND_bytes(random_data.data(), 32) != 1) {
        // 如果RAND_bytes失败，使用替代方法
        std::cerr << "Warning: RAND_bytes failed, using alternative method for token generation" << std::endl;
        for (size_t i = 0; i < 32; i++) {
            random_data[i] = static_cast<unsigned char>(rand() % 256);
        }
    }
    
    // 转换为十六进制字符串
    std::stringstream ss;
    for (size_t i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)random_data[i];
    }
    
    return ss.str();
}

// 兼容旧接口
std::string sha256(const std::string& str) {
    return CryptoUtils::sha256(str);
}