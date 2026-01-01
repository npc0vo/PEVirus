@echo off
REM PEVirus Malware Detector - 运行脚本
REM 自动扫描 Tests 目录并生成报告

cd /d "%~dp0"

echo ========================================
echo   PEVirus Malware Detector
echo   正在安装依赖...
echo ========================================

pip install yara-python pefile -q

echo.
echo 开始扫描 Tests 目录...
echo.

python malware_detector.py -t "../Tests" -v -o scan_report.json

echo.
echo ========================================
echo   扫描完成！
echo   报告已保存到: scan_report.json
echo ========================================

pause
