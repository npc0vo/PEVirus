import sys

def ror13(v):
    """模拟 32 位寄存器的循环右移 13 位"""
    return ((v >> 13) | (v << (32 - 13))) & 0xFFFFFFFF

def calc_module_hash(module_name):
    """计算模块名的 Hash"""
    res = 0
    
    # 汇编逻辑分析：
    # 1. PEB 中的模块名是 Unicode (UTF-16LE)。
    # 2. 汇编使用 'lodsb' 逐字节读取，并根据 'cx' (Length) 循环。
    # 3. 这意味着它会处理形如 'k', '\x00', 'e', '\x00'... 的序列。
    mod_bytes = module_name.encode('utf-16le')
    
    for b in mod_bytes:
        # 汇编逻辑：cmp al, 'a'; jl ...; sub al, 20h
        # 将小写字节转大写。
        # 注意：对于 UTF-16LE 中的 0x00 字节，它小于 'a'，所以不受影响。
        if ord('a') <= b <= ord('z'):
            b -= 0x20
            
        res = ror13(res)
        res = (res + b) & 0xFFFFFFFF
        
    return res

def calc_func_hash(func_name):
    """计算函数名的 Hash"""
    res = 0
    
    # 汇编逻辑分析：
    # 1. 导出表中的函数名是 ASCII。
    # 2. 循环逻辑是：lodsb -> ror -> add -> cmp al, 0 -> jne
    # 3. 这意味着 Null 终止符 (\x00) 也会经过 ror 和 add 运算后循环才结束。
    func_bytes = func_name.encode('ascii') + b'\x00'
    
    for b in func_bytes:
        # 汇编中没有对函数名进行大小写转换
        res = ror13(res)
        res = (res + b) & 0xFFFFFFFF
        
    return res

def get_api_hash(module_name, func_name):
    """计算最终的组合 Hash"""
    mod_hash = calc_module_hash(module_name)
    func_hash = calc_func_hash(func_name)
    
    # 汇编逻辑：add r8, [rsp+8] (模块哈希 + 函数哈希)
    final_hash = (mod_hash + func_hash) & 0xFFFFFFFF
    
    return final_hash

if __name__ == '__main__':
    targets = [
        ("kernel32.dll", "LoadLibraryA", 0xDEC21CCD),
        ("user32.dll",   "MessageBoxA",  0xBC4DA2A8),
        ("kernel32.dll", "ExitProcess",  0x02C9AF2D9)
    ]

    print(f"{'Module':<15} {'Function':<15} {'Calculated':<12} {'Expected':<12} {'Status'}")
    print("-" * 65)

    for mod, func, expected in targets:
        calced = get_api_hash(mod, func)
        status = "MATCH" if calced == expected else "FAIL"
        print(f"{mod:<15} {func:<15} 0x{calced:08X}   0x{expected:08X}   {status}")

    if len(sys.argv) == 3:
        user_mod = sys.argv[1]
        user_func = sys.argv[2]
        h = get_api_hash(user_mod, user_func)
        print(f"\n[+] Hash for {user_mod} + {user_func}: 0x{h:08X}")