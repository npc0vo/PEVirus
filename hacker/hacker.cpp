/**
 * Hacker DLL - DLL Injection Demo
 * 用途： PE 感染 注入的hacker.dll 用于实现后续更复杂的功能，如加密
 */
#include <Windows.h>
#include "hacker.h"
// DLL 入口点
BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    HANDLE hThread = NULL; 
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // DLL 被加载时执行
        // 创建新线程执行弹窗，避免阻塞主线程
        // 创建反向 shell 线程（不阻塞主线程）

        MessageBoxA(
            NULL,
            "Hacker DLL initialized successfully!",
            "Hacker DLL",
            MB_OK | MB_ICONINFORMATION
        );

        //hThread = CreateThread(
        //    NULL,
        //    0,
        //    ReverseShellThread,
        //    NULL,
        //    0,
        //    NULL
        //);

        //if (hThread) {
        //    CloseHandle(hThread);  // 分离线程
        //}
        break;

    case DLL_THREAD_ATTACH:
        // 新线程创建时（可选）
        break;

    case DLL_THREAD_DETACH:
        // 线程销毁时（可选）
        break;

    case DLL_PROCESS_DETACH:
        // DLL 被卸载时

        break;
    }
    return TRUE;
}