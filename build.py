#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PEVirus Build Script
功能:
1. 编译生成 shellcode.exe
2. 从 shellcode.exe 提取 .text 段数据到 PEVirus.h
3. 编译生成 PEVirus.exe
4. 编译生成 hacker.dll
5. 复制生成目录的 PEVirus.exe 和 hacker.dll 到 Tests 目录
"""

import os
import sys
import subprocess
import struct
import shutil
from pathlib import Path

# 配置 - Visual Studio 2026 路径
VS_PATH = r"D:\Program Files\Microsoft Visual Studio\18\Community"
ML64_PATH = rf"{VS_PATH}\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\ml64.exe"
LINK_PATH = rf"{VS_PATH}\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\link.exe"
CMAKE_PATH = rf"{VS_PATH}\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
NINJA_PATH = rf"{VS_PATH}\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

# vcpkg 配置
VCPKG_ROOT = Path.home() / "vcpkg"
VCPKG_TOOLCHAIN = VCPKG_ROOT / "scripts" / "buildsystems" / "vcpkg.cmake"

# 项目路径
SCRIPT_DIR = Path(__file__).parent.resolve()
SHELLCODE_DIR = SCRIPT_DIR / "shellcode"
SHELLCODE_ASM = SHELLCODE_DIR / "shellcode.asm"
SHELLCODE_OBJ = SHELLCODE_DIR / "shellcode.obj"
SHELLCODE_EXE = SHELLCODE_DIR / "shellcode.exe"
PEVIRUS_H = SCRIPT_DIR / "PEVirus.h"
BUILD_DIR = SCRIPT_DIR / "out" / "build" / "x64-release"
TESTS_DIR = SCRIPT_DIR / "Tests"

# ANSI 颜色代码
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def print_info(msg):
    print(f"{Colors.OKBLUE}[*]{Colors.ENDC} {msg}")

def print_success(msg):
    print(f"{Colors.OKGREEN}[+]{Colors.ENDC} {msg}")

def print_error(msg):
    print(f"{Colors.FAIL}[-]{Colors.ENDC} {msg}")

def print_warning(msg):
    print(f"{Colors.WARNING}[!]{Colors.ENDC} {msg}")

def setup_vs_environment():
    """设置 Visual Studio 编译环境变量"""
    print_info("Setting up Visual Studio environment...")
    
    # 基本路径
    vc_tools = rf"{VS_PATH}\VC\Tools\MSVC\14.50.35717"
    windows_sdk = r"D:\Windows Kits\10"
    sdk_version = "10.0.26100.0"
    
    # 设置 INCLUDE 路径
    include_paths = [
        rf"{vc_tools}\include",
        rf"{vc_tools}\ATLMFC\include",
        rf"{VS_PATH}\VC\Auxiliary\VS\include",
        rf"{windows_sdk}\include\{sdk_version}\ucrt",
        rf"{windows_sdk}\include\{sdk_version}\um",
        rf"{windows_sdk}\include\{sdk_version}\shared",
        rf"{windows_sdk}\include\{sdk_version}\winrt",
        rf"{windows_sdk}\include\{sdk_version}\cppwinrt"
    ]
    os.environ['INCLUDE'] = ';'.join(include_paths)
    
    # 设置 LIB 路径
    lib_paths = [
        rf"{vc_tools}\ATLMFC\lib\x64",
        rf"{vc_tools}\lib\x64",
        rf"{windows_sdk}\lib\{sdk_version}\ucrt\x64",
        rf"{windows_sdk}\lib\{sdk_version}\um\x64"
    ]
    os.environ['LIB'] = ';'.join(lib_paths)
    
    # 设置 PATH
    path_additions = [
        rf"{vc_tools}\bin\Hostx64\x64",
        rf"{VS_PATH}\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin",
        rf"{VS_PATH}\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
        rf"{windows_sdk}\bin\{sdk_version}\x64",
        rf"{windows_sdk}\bin\x64"
    ]
    current_path = os.environ.get('PATH', '')
    os.environ['PATH'] = ';'.join(path_additions) + ';' + current_path
    
    print_success("Visual Studio environment configured")

def run_command(cmd, cwd=None, shell=True):
    """运行命令并返回结果"""
    print_info(f"Running: {cmd}")
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            shell=shell,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        
        if result.stdout:
            pass
            #print(result.stdout)
        if result.stderr:
            print(result.stderr)
            
        if result.returncode != 0:
            print_error(f"Command failed with return code {result.returncode}")
            return False
        return True
    except Exception as e:
        print_error(f"Exception running command: {e}")
        return False

def step1_build_shellcode():
    """步骤1: 编译生成 shellcode.exe"""
    print_info("=" * 60)
    print_info("Step 1: Building shellcode.exe")
    print_info("=" * 60)
    
    # 检查文件是否存在
    if not SHELLCODE_ASM.exists():
        print_error(f"shellcode.asm not found: {SHELLCODE_ASM}")
        return False
    
    # 检查工具是否存在
    if not Path(ML64_PATH).exists():
        print_error(f"ml64.exe not found: {ML64_PATH}")
        return False
    
    if not Path(LINK_PATH).exists():
        print_error(f"link.exe not found: {LINK_PATH}")
        return False
    
    # 删除旧的输出文件
    for f in [SHELLCODE_OBJ, SHELLCODE_EXE]:
        if f.exists():
            f.unlink()
            print_info(f"Removed old file: {f}")
    
    # 编译汇编文件
    print_info("Assembling shellcode.asm...")
    cmd = f'"{ML64_PATH}" /c /Fo "{SHELLCODE_OBJ}" "{SHELLCODE_ASM}"'
    if not run_command(cmd, cwd=SHELLCODE_DIR):
        return False
    
    if not SHELLCODE_OBJ.exists():
        print_error("Failed to generate shellcode.obj")
        return False
    print_success("shellcode.obj generated")
    
    # 链接生成可执行文件
    print_info("Linking shellcode.exe...")
    cmd = f'"{LINK_PATH}" /ENTRY:main /SUBSYSTEM:CONSOLE /OUT:"{SHELLCODE_EXE}" "{SHELLCODE_OBJ}" kernel32.lib user32.lib'
    if not run_command(cmd, cwd=SHELLCODE_DIR):
        return False
    
    if not SHELLCODE_EXE.exists():
        print_error("Failed to generate shellcode.exe")
        return False
    print_success(f"shellcode.exe generated: {SHELLCODE_EXE}")
    
    return True

def parse_pe_header(file_path):
    """解析PE文件头获取节表信息"""
    with open(file_path, 'rb') as f:
        # 读取DOS头
        dos_header = f.read(64)
        e_lfanew = struct.unpack('<I', dos_header[60:64])[0]
        
        # 跳到PE头
        f.seek(e_lfanew)
        pe_signature = f.read(4)
        if pe_signature != b'PE\x00\x00':
            raise ValueError("Not a valid PE file")
        
        # 读取COFF头
        coff_header = f.read(20)
        number_of_sections = struct.unpack('<H', coff_header[2:4])[0]
        size_of_optional_header = struct.unpack('<H', coff_header[16:18])[0]
        
        # 跳过Optional头
        f.seek(e_lfanew + 24 + size_of_optional_header)
        
        # 读取节表
        sections = []
        for _ in range(number_of_sections):
            section_header = f.read(40)
            name = section_header[0:8].rstrip(b'\x00').decode('ascii', errors='ignore')
            virtual_size = struct.unpack('<I', section_header[8:12])[0]
            virtual_address = struct.unpack('<I', section_header[12:16])[0]
            size_of_raw_data = struct.unpack('<I', section_header[16:20])[0]
            pointer_to_raw_data = struct.unpack('<I', section_header[20:24])[0]
            
            sections.append({
                'name': name,
                'virtual_size': virtual_size,
                'virtual_address': virtual_address,
                'size_of_raw_data': size_of_raw_data,
                'pointer_to_raw_data': pointer_to_raw_data
            })
        
        return sections

def step2_extract_text_section():
    """步骤2: 从 shellcode.exe 提取 .text 段数据到 PEVirus.h"""
    print_info("=" * 60)
    print_info("Step 2: Extracting .text section from shellcode.exe")
    print_info("=" * 60)
    
    if not SHELLCODE_EXE.exists():
        print_error(f"shellcode.exe not found: {SHELLCODE_EXE}")
        return False
    
    try:
        # 解析PE文件
        sections = parse_pe_header(SHELLCODE_EXE)
        
        # 查找.text节
        text_section = None
        for section in sections:
            print_info(f"Found section: {section['name']} (Size: {section['size_of_raw_data']} bytes)")
            if section['name'] == '.text':
                text_section = section
                break
        
        if not text_section:
            print_error(".text section not found in shellcode.exe")
            return False
        
        print_success(f".text section found at offset 0x{text_section['pointer_to_raw_data']:X}")
        print_info(f"Size: {text_section['size_of_raw_data']} bytes")
        
        # 读取.text段数据
        with open(SHELLCODE_EXE, 'rb') as f:
            f.seek(text_section['pointer_to_raw_data'])
            text_data = f.read(text_section['size_of_raw_data'])
        
        # 移除末尾的填充字节(0x00)
        text_data = text_data.rstrip(b'\x00')
        
        print_success(f"Extracted {len(text_data)} bytes from .text section")
        
        # 生成C数组格式
        hex_string = ', '.join(f'0x{b:02X}' for b in text_data)
        
        # 格式化为每行16个字节
        bytes_per_line = 16
        formatted_lines = []
        for i in range(0, len(text_data), bytes_per_line):
            line_bytes = text_data[i:i+bytes_per_line]
            line = '    ' + ', '.join(f'0x{b:02X}' for b in line_bytes)
            if i + bytes_per_line < len(text_data):
                line += ','
            formatted_lines.append(line)
        
        formatted_hex = '\n'.join(formatted_lines)
        
        # 生成PEVirus.h内容
        header_content = f"""// PEVirus.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once
uint8_t shellcode[] = {{
{formatted_hex}
}};
// TODO: 在此处引用程序需要的其他标头。
"""
        
        # 写入PEVirus.h
        with open(PEVIRUS_H, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print_success(f"PEVirus.h updated with {len(text_data)} bytes shellcode")
        print_info(f"File: {PEVIRUS_H}")
        
        return True
        
    except Exception as e:
        print_error(f"Failed to extract .text section: {e}")
        import traceback
        traceback.print_exc()
        return False

def step3_build_pevirus():
    """步骤3: 编译生成 PEVirus.exe"""
    print_info("=" * 60)
    print_info("Step 3: Building PEVirus.exe with CMake")
    print_info("=" * 60)
    
    # 检查 CMake 和 Ninja 是否存在
    if not Path(CMAKE_PATH).exists():
        print_error(f"CMake not found: {CMAKE_PATH}")
        return False
    
    if not Path(NINJA_PATH).exists():
        print_error(f"Ninja not found: {NINJA_PATH}")
        return False
    
    # 创建build目录
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    # 配置CMake - 使用与 Visual Studio 相同的配置
    print_info("Configuring CMake...")
    
    cmake_cmd = [
        f'"{CMAKE_PATH}"',
        '-G', '"Ninja"',
        f'-DCMAKE_C_COMPILER:STRING="cl.exe"',
        f'-DCMAKE_CXX_COMPILER:STRING="cl.exe"',
        f'-DCMAKE_BUILD_TYPE:STRING="Release"',
        f'-DCMAKE_TOOLCHAIN_FILE:STRING="{VCPKG_TOOLCHAIN}"',
        f'-DVCPKG_TARGET_TRIPLET:STRING="x64-windows-static"',
        f'-DCMAKE_MAKE_PROGRAM="{NINJA_PATH}"',
        f'"{SCRIPT_DIR}"'
    ]
    
    cmd = ' '.join(cmake_cmd)
    if not run_command(cmd, cwd=BUILD_DIR):
        print_error("CMake configuration failed")
        return False
    
    print_success("CMake configuration completed")
    
    # 编译
    print_info("Building with Ninja...")
    cmd = f'"{CMAKE_PATH}" --build "{BUILD_DIR}" --config Release'
    if not run_command(cmd):
        print_error("Build failed")
        return False
    
    # 检查PEVirus.exe是否生成
    pevirus_exe = BUILD_DIR / "PEVirus.exe"
    if not pevirus_exe.exists():
        print_error(f"PEVirus.exe not found in {BUILD_DIR}")
        return False
    
    print_success(f"PEVirus.exe generated: {pevirus_exe}")
    return True

def step4_build_hacker_dll():
    """步骤4: 编译生成 hacker.dll (通过CMake一起编译)"""
    print_info("=" * 60)
    print_info("Step 4: Checking hacker.dll")
    print_info("=" * 60)
    
    # hacker.dll应该已经在step3中编译完成
    hacker_dll = BUILD_DIR / "hacker.dll"
    if not hacker_dll.exists():
        print_error(f"hacker.dll not found in {BUILD_DIR}")
        return False
    
    print_success(f"hacker.dll found: {hacker_dll}")
    return True

def step5_copy_to_tests():
    """步骤5: 复制 PEVirus.exe 和 hacker.dll 到 Tests 目录"""
    print_info("=" * 60)
    print_info("Step 5: Copying files to Tests directory")
    print_info("=" * 60)
    
    # 创建Tests目录
    TESTS_DIR.mkdir(exist_ok=True)
    print_info(f"Tests directory: {TESTS_DIR}")
    
    # 复制PEVirus.exe
    pevirus_exe = BUILD_DIR / "PEVirus.exe"
    if pevirus_exe.exists():
        dest = TESTS_DIR / "PEVirus.exe"
        shutil.copy2(pevirus_exe, dest)
        print_success(f"Copied PEVirus.exe to {dest}")
    else:
        print_error(f"PEVirus.exe not found: {pevirus_exe}")
        return False
    
    # 复制hacker.dll
    hacker_dll = BUILD_DIR / "hacker.dll"
    if hacker_dll.exists():
        dest = TESTS_DIR / "hacker.dll"
        shutil.copy2(hacker_dll, dest)
        print_success(f"Copied hacker.dll to {dest}")
    else:
        print_error(f"hacker.dll not found: {hacker_dll}")
        return False
    
    # 同时复制Victim.exe和injector.exe到Tests目录(如果存在)
    for exe_name in ["Victim.exe", "injector.exe"]:
        exe_path = BUILD_DIR / exe_name
        if exe_path.exists():
            dest = TESTS_DIR / exe_name
            shutil.copy2(exe_path, dest)
            print_success(f"Copied {exe_name} to {dest}")
    
    return True

def main():
    """主函数"""
    print_info("=" * 60)
    print_info(f"{Colors.BOLD}PEVirus Build Script{Colors.ENDC}")
    print_info("Author: npc0vo")
    print_info("=" * 60)
    print()
    
    # 设置 Visual Studio 环境
    setup_vs_environment()
    print()
    
    # 步骤1: 编译shellcode.exe
    if not step1_build_shellcode():
        print_error("Step 1 failed!")
        return 1
    print()
    
    # 步骤2: 提取.text段
    if not step2_extract_text_section():
        print_error("Step 2 failed!")
        return 1
    print()
    
    # 步骤3: 编译PEVirus.exe
    if not step3_build_pevirus():
        print_error("Step 3 failed!")
        return 1
    print()
    
    # 步骤4: 检查hacker.dll
    if not step4_build_hacker_dll():
        print_error("Step 4 failed!")
        return 1
    print()
    
    # 步骤5: 复制到Tests目录
    if not step5_copy_to_tests():
        print_error("Step 5 failed!")
        return 1
    print()
    
    print_info("=" * 60)
    print_success(f"{Colors.BOLD}Build completed successfully!{Colors.ENDC}")
    print_info("=" * 60)
    print()
    print_info("Generated files:")
    print_info(f"  - {SHELLCODE_EXE}")
    print_info(f"  - {BUILD_DIR / 'PEVirus.exe'}")
    print_info(f"  - {BUILD_DIR / 'hacker.dll'}")
    print_info(f"  - {TESTS_DIR / 'PEVirus.exe'}")
    print_info(f"  - {TESTS_DIR / 'hacker.dll'}")
    print()
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print()
        print_warning("Build interrupted by user")
        sys.exit(1)
    except Exception as e:
        print_error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
