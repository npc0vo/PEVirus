/**
 * DLL Injector
 * 功能：将 hacker.dll 注入到目标进程
 * 用法：injector.exe <target_process_id>
 */

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

// 通过进程名查找进程 ID
DWORD FindProcessId(const std::wstring& processName) {
    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            if (processName == processEntry.szExeFile) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return 0;
}

// 注入 DLL 到目标进程
bool InjectDLL(DWORD processId, const std::string& dllPath) {
    std::cout << "[*] Target Process ID: " << processId << std::endl;
    std::cout << "[*] DLL Path: " << dllPath << std::endl;

    // 1. 打开目标进程
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE,
        processId
    );

    if (!hProcess) {
        std::cerr << "[-] Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "[+] Process opened successfully" << std::endl;

    // 2. 在目标进程中分配内存
    SIZE_T dllPathSize = dllPath.length() + 1;
    LPVOID pRemoteDllPath = VirtualAllocEx(
        hProcess,
        NULL,
        dllPathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!pRemoteDllPath) {
        std::cerr << "[-] Failed to allocate memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "[+] Memory allocated at: 0x" << std::hex << pRemoteDllPath << std::dec << std::endl;

    // 3. 写入 DLL 路径
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(
        hProcess,
        pRemoteDllPath,
        dllPath.c_str(),
        dllPathSize,
        &bytesWritten
    )) {
        std::cerr << "[-] Failed to write memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "[+] DLL path written (" << bytesWritten << " bytes)" << std::endl;

    // 4. 获取 LoadLibraryA 地址
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPVOID pLoadLibraryA = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");

    if (!pLoadLibraryA) {
        std::cerr << "[-] Failed to get LoadLibraryA address" << std::endl;
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "[+] LoadLibraryA address: 0x" << std::hex << pLoadLibraryA << std::dec << std::endl;

    // 5. 创建远程线程
    HANDLE hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)pLoadLibraryA,
        pRemoteDllPath,
        0,
        NULL
    );

    if (!hThread) {
        std::cerr << "[-] Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "[+] Remote thread created successfully" << std::endl;
    std::cout << "[*] Waiting for DLL to load..." << std::endl;

    // 等待线程完成
    WaitForSingleObject(hThread, INFINITE);

    // 获取线程退出码（LoadLibrary 的返回值，即 DLL 模块句柄）
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);

    if (exitCode == 0) {
        std::cerr << "[-] DLL injection failed (LoadLibrary returned NULL)" << std::endl;
    } else {
        std::cout << "[+] DLL injected successfully! Module handle: 0x" 
                  << std::hex << exitCode << std::dec << std::endl;
    }

    // 清理
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return exitCode != 0;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "   DLL Injector - by npc0vo\n";
    std::cout << "========================================\n\n";

    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <process_id>           - Inject by PID\n";
        std::cout << "  " << argv[0] << " <process_name.exe>     - Inject by name\n";
        std::cout << "\nExample:\n";
        std::cout << "  " << argv[0] << " 1234\n";
        std::cout << "  " << argv[0] << " notepad.exe\n";
        return 1;
    }

    // 获取 DLL 完整路径（假设在同目录下）
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string dllPath = std::string(currentDir) + "\\hacker.dll";

    // 检查 DLL 是否存在
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "[-] DLL not found: " << dllPath << std::endl;
        std::cerr << "[-] Make sure hacker.dll is in the same directory as the injector" << std::endl;
        return 1;
    }

    // 判断是 PID 还是进程名
    DWORD targetPid = 0;
    std::string arg = argv[1];

    // 尝试解析为数字（PID）
    try {
        targetPid = std::stoul(arg);
    } catch (...) {
        // 不是数字，尝试作为进程名查找
        std::wstring processName(arg.begin(), arg.end());
        targetPid = FindProcessId(processName);
        
        if (targetPid == 0) {
            std::cerr << "[-] Process not found: " << arg << std::endl;
            return 1;
        }
        
        std::cout << "[+] Found process '" << arg << "' with PID: " << targetPid << std::endl;
    }

    // 执行注入
    if (InjectDLL(targetPid, dllPath)) {
        std::cout << "\n[+] Injection completed successfully!" << std::endl;
        std::cout << "[*] Check the target process for message boxes" << std::endl;
        return 0;
    } else {
        std::cerr << "\n[-] Injection failed!" << std::endl;
        return 1;
    }
}
