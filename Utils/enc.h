/**
 * AES Encryption Utility
 * 简单的文件加密工具
 */

#ifndef AES_H
#define AES_H

#include <Windows.h>
#include <wincrypt.h>
#include <string>
#include <vector>

#pragma comment(lib, "advapi32.lib")

class SimpleEncryptor {
public:
    /**
     * 加密文件
     * @param filePath 文件路径
     * @param password 加密密码
     * @return 成功返回 true
     */
    static bool EncryptFile(const std::string& filePath, const std::string& password);

    /**
     * 解密文件
     * @param filePath 文件路径
     * @param password 解密密码
     * @return 成功返回 true
     */
    static bool DecryptFile(const std::string& filePath, const std::string& password);

    /**
     * 批量加密目录下的文件
     * @param directory 目录路径
     * @param extensions 文件扩展名列表（如 {".txt", ".doc"}）
     * @param password 加密密码
     * @return 加密的文件数量
     */
    static int EncryptDirectory(const std::string& directory, 
                                const std::vector<std::string>& extensions,
                                const std::string& password);

private:
    // XOR 简单加密
    static void XorEncrypt(std::vector<BYTE>& data, const std::string& key);
	static void AESEncrypt(std::vector<BYTE>& data, const std::string& key);
};

#endif // AES_H
