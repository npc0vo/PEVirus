#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
快速测试脚本 - 验证检测器功能
"""

import sys
import os

# 添加当前目录到路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from malware_detector import MalwareDetector, ReportGenerator
from pathlib import Path

def main():
    print("=" * 60)
    print("  PEVirus Malware Detector - 快速测试")
    print("=" * 60)
    print()
    
    # 检查依赖
    try:
        import yara
        print("[?] yara-python 已安装")
    except ImportError:
        print("[?] yara-python 未安装，请运行: pip install yara-python")
    
    try:
        import pefile
        print("[?] pefile 已安装")
    except ImportError:
        print("[?] pefile 未安装，请运行: pip install pefile")
    
    print()
    
    # 初始化检测器
    rules_path = Path(__file__).parent / "rules"
    if rules_path.exists():
        print(f"[?] YARA规则目录存在: {rules_path}")
    else:
        print(f"[?] YARA规则目录不存在: {rules_path}")
        return
    
    print()
    print("正在初始化检测器...")
    detector = MalwareDetector(str(rules_path))
    
    # 扫描 Tests 目录
    tests_path = Path(__file__).parent.parent / "Tests"
    if tests_path.exists():
        print(f"\n正在扫描: {tests_path}")
        print("-" * 60)
        
        reports = detector.scan_directory(str(tests_path))
        
        print("\n" + "-" * 60)
        print(ReportGenerator.generate_summary_report(reports))
        
        # 保存报告
        output_path = Path(__file__).parent / "scan_report.json"
        json_report = ReportGenerator.generate_json_report(reports)
        output_path.write_text(json_report, encoding='utf-8')
        print(f"\n[?] JSON报告已保存到: {output_path}")
        
    else:
        print(f"[!] Tests目录不存在: {tests_path}")
    
    print()
    print("测试完成！")

if __name__ == '__main__':
    main()
