# PEVirus 恶意代码检测器

## 项目概述

本项目实现了一个综合恶意代码检测工具，主要针对 PEVirus 病毒进行检测。采用多种检测方法结合的方式，包括：

1. **YARA 规则特征码检测** - 基于已知恶意代码特征进行匹配
2. **PE 结构分析** - 检测可疑节区、导入表等
3. **启发式分析** - 检测熵值异常、可疑字符串组合等

## 文件结构

```
detector/
├── malware_detector.py      # 主检测程序
├── requirements.txt         # Python依赖
├── run_scan.bat            # Windows运行脚本
├── README.md               # 本文档
├── rules/                  # YARA规则目录
│   └── pevirus_rules.yar   # PEVirus检测规则
└── scan_report.json        # 扫描报告（运行后生成）
```

## 安装依赖

```bash
pip install -r requirements.txt
```

或者直接安装：
```bash
pip install yara-python pefile
```

## 使用方法

### 基本使用

```bash
# 扫描 Tests 目录
python malware_detector.py -t ../Tests

# 扫描单个文件
python malware_detector.py -t victim.exe

# 输出详细信息并保存JSON报告
python malware_detector.py -t ../Tests -v -o scan_report.json
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `-t, --target` | 要扫描的文件或目录（必需） |
| `-r, --rules` | YARA规则路径（默认: rules） |
| `-o, --output` | 输出JSON报告到文件 |
| `-v, --verbose` | 详细输出模式 |
| `--no-yara` | 禁用YARA扫描 |
| `--no-pe` | 禁用PE分析 |
| `--no-recursive` | 不递归扫描子目录 |

### 快速运行（Windows）

直接双击 `run_scan.bat` 即可自动安装依赖并扫描 Tests 目录。

## 检测方法详解

### 1. YARA 规则检测

YARA 是一种用于恶意软件研究的模式匹配工具。本项目包含针对 PEVirus 的自定义规则：

- **PEVirus_Hacked_Section** - 检测 `.hacked` 感染节区标记
- **PEVirus_Mutex** - 检测 `PEVirus_Mutex_npc0vo` 互斥体
- **PEVirus_Registry_Persistence** - 检测注册表持久化
- **PEVirus_Shellcode_Pattern** - 检测 Shellcode 模式
- **PEVirus_Daemon_Behavior** - 检测后台守护行为
- **PEVirus_Full_Detection** - 综合检测规则

**优点：**
- 检测速度快，准确率高
- 规则易于编写和维护
- 支持复杂的匹配逻辑

**局限性：**
- 依赖已知特征，无法检测未知变种
- 容易被混淆/加壳绕过
- 可能产生误报

### 2. PE 结构分析

对 PE 文件进行深度解析，检测异常：

- **节区名称检测** - 检测 `.hacked`, `.evil`, `.virus` 等可疑节区
- **高熵值节区** - 熵值 > 7.0 可能表示加壳或加密
- **可疑 API 导入** - 检测 `CreateRemoteThread`, `WriteProcessMemory` 等

**优点：**
- 可以发现结构层面的异常
- 不依赖具体特征
- 可检测加壳迹象

**局限性：**
- 仅适用于 PE 文件
- 误报率相对较高
- 无法检测内存中的恶意行为

### 3. 启发式分析

基于行为和特征组合进行推断：

- **熵值分析** - 高熵值可能表示加密/压缩
- **字符串特征** - 搜索 PEVirus 特有字符串
- **Shellcode 模式** - 检测常见 Shellcode 字节序列

**优点：**
- 可能检测到变种和未知威胁
- 综合多种指标
- 弥补特征码检测的不足

**局限性：**
- 误报率相对较高
- 计算开销较大
- 需要精心调整阈值

## PEVirus 特征总结

根据源码分析，PEVirus 具有以下特征：

### 感染特征
- 添加 `.hacked` 节区存放 Shellcode
- 修改入口点指向新节区
- 在新节区末尾跳回原入口点

### 持久化特征
- 注册表启动项: `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\SystemUpdate`
- 启动文件夹复制: `%STARTUP%\SystemUpdate.exe`
- 备份副本: `%TEMP%\svchost_backup.exe`

### 后台特征
- 互斥体: `Global\PEVirus_Mutex_npc0vo`
- 守护线程定期检查持久化状态
- 支持 C2 反向 Shell 连接

### 字符串特征
- "Infection Successful"
- "Total files infected"
- "Background daemon"
- "Persistence lost"
- "Repairing persistence"

## 威胁等级说明

| 等级 | 说明 |
|------|------|
| ? Clean | 未检测到威胁 |
| ?? Low | 低风险，可能是误报 |
| ?? Medium | 中等风险，需要进一步分析 |
| ?? High | 高风险，很可能是恶意软件 |
| ?? Critical | 严重威胁，确认为恶意软件 |

## 检测能力边界分析

### 检测能力
? 可以检测：
- 未加壳的 PEVirus 原始样本
- 包含明显特征字符串的变种
- 具有 `.hacked` 节区的感染文件
- 使用相同 Shellcode 的变种

### 检测局限
? 可能无法检测：
- 加壳/混淆后的样本
- 字符串加密的变种
- 修改节区名称的变种
- 完全重写的新变种
- 内存驻留型恶意软件

### 误报分析
可能的误报情况：
- 使用 UPX 等常见加壳工具的正常软件
- 包含相似 API 调用的安全工具
- 具有高熵值的加密/压缩数据

## 性能开销

| 检测方法 | 相对开销 | 说明 |
|----------|----------|------|
| YARA | 低 | 高效的模式匹配算法 |
| PE分析 | 中 | 需要解析完整PE结构 |
| 启发式 | 中-高 | 需要读取完整文件计算熵值 |

典型扫描速度：
- 小文件（< 1MB）：< 100ms
- 中等文件（1-10MB）：100-500ms
- 大文件（> 10MB）：> 500ms

## 改进建议

1. **增加行为检测** - 在沙箱中运行并监控API调用
2. **机器学习** - 训练模型识别恶意特征
3. **云端查询** - 集成在线恶意软件数据库
4. **实时监控** - 文件系统监控和进程注入检测

## 参考资料

- [YARA Documentation](https://yara.readthedocs.io/)
- [pefile Documentation](https://github.com/erocarrera/pefile)
- [PE Format Specification](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
