@echo off
REM Shellcode 编译脚本
REM 需要安装 ml64.exe (Microsoft Macro Assembler for x64)

echo [*] Building x64 shellcode from shellcode.asm...

REM 使用 MASM (ml64) 编译汇编代码
"D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\ml64" /c /Fo shellcode.obj shellcode.asm
if errorlevel 1 (
    echo [-] Assembly compilation failed
    pause
    goto :eof
)

echo [+] shellcode.obj generated

REM 链接生成可执行文件
link /ENTRY:main /SUBSYSTEM:CONSOLE /OUT:shellcode.exe shellcode.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [-] Linking failed
    pause
    goto :eof
)

echo [+] shellcode.exe generated

echo [+] Build complete!
echo [*] Run shellcode.exe to test the MessageBox
pause
