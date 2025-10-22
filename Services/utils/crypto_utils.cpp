#include "crypto_utils.h"
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <cstring>

// 使用OpenSSL 3.0的新API实现SHA256哈希函数
std::string sha256(const std::string& str) {
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