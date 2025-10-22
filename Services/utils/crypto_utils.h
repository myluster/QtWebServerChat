#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>

// 使用OpenSSL 3.0的新API
std::string sha256(const std::string& str);

#endif // CRYPTO_UTILS_H