@echo off
echo [*] Cleaning up PEVirus traces...

REM 停止进程
taskkill /f /im PEVirus.exe 2>nul
taskkill /f /im SystemUpdate.exe 2>nul

REM 删除注册表
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SystemUpdate /f 2>nul

REM 删除启动目录文件
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\SystemUpdate.exe" /f 2>nul

echo [+] Cleanup complete!
pause