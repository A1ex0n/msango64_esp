option casemap:none

EXTERN g_GameVars:BYTE
EXTERN g_CurrrentObject:QWORD
EXTERN g_HeroWorldPoint:QWORD
EXTERN pRelWritePetPoint:QWORD
EXTERN cmpfloat:PROC

.code

PUBLIC HookPetMove
HookPetMove:
    ; Hook 函数需要保存所有可能被修改的寄存器
    ; 保存通用寄存器（按 x64 调用约定，需要保存非易失寄存器，但hook代码中应该保存所有可能被修改的）
    push    rax
    push    rcx
    push    rdx
    push    rbx
    push    rsi
    push    rdi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    rbp
    
    ; 保存 XMM 寄存器（如果使用了浮点运算）
    sub     rsp, 100h    ; 为 XMM0-XMM15 分配空间（16个寄存器 * 16字节 = 256字节 = 100h）
    movdqa  [rsp+00h], xmm0
    movdqa  [rsp+10h], xmm1
    movdqa  [rsp+20h], xmm2
    movdqa  [rsp+30h], xmm3
    movdqa  [rsp+40h], xmm4
    movdqa  [rsp+50h], xmm5
    movdqa  [rsp+60h], xmm6
    movdqa  [rsp+70h], xmm7
    movdqa  [rsp+80h], xmm8
    movdqa  [rsp+90h], xmm9
    movdqa  [rsp+0A0h], xmm10
    movdqa  [rsp+0B0h], xmm11
    movdqa  [rsp+0C0h], xmm12
    movdqa  [rsp+0D0h], xmm13
    movdqa  [rsp+0E0h], xmm14
    movdqa  [rsp+0F0h], xmm15
    
    ; if (!g_GameVars.isGet) goto skip_logic;
    mov     al, BYTE PTR [g_GameVars]    ; GameVariables::isGet 位于结构体首字节
    test    al, al
    jz      skip_logic

    ; if (rbx != g_CurrrentObject) goto skip_logic;
    mov     rax, QWORD PTR [g_CurrrentObject]
    cmp     rbx, rax
    jne     skip_logic

    ; 调用 cmpfloat 判断是否在老家角落
    ; x64 调用约定：浮点参数用 XMM0, XMM1
    ; 加载 x 坐标到 XMM0 (float)
    movss   xmm0, DWORD PTR [rsi+110h]
    
    ; 加载 y 坐标到 XMM1 (float)
    movss   xmm1, DWORD PTR [rsi+114h]
    
    ; 调用 cmpfloat(x, y)
    ; x64 调用约定：浮点参数在 XMM0, XMM1
    sub     rsp, 20h    ; 分配 shadow space (32 bytes)
    call    cmpfloat
    add     rsp, 20h    ; 恢复栈
    
    ; 检查返回值：如果 eax == 0（不在老家角落），则记录坐标
    test    eax, eax
    jnz     skip_logic  ; 如果返回值非0（在老家角落），跳过记录
    
    ; 记录坐标：
    ; X = [rsi+110h]
    ; Y = [rsi+114h]
    mov     eax, DWORD PTR [rsi+110h]
    mov     DWORD PTR [g_HeroWorldPoint], eax       ; g_HeroWorldPoint.x

    mov     eax, DWORD PTR [rsi+114h]
    mov     DWORD PTR [g_HeroWorldPoint+4], eax     ; g_HeroWorldPoint.y

skip_logic:
    ; 恢复 XMM 寄存器
    movdqa  xmm0, [rsp+00h]
    movdqa  xmm1, [rsp+10h]
    movdqa  xmm2, [rsp+20h]
    movdqa  xmm3, [rsp+30h]
    movdqa  xmm4, [rsp+40h]
    movdqa  xmm5, [rsp+50h]
    movdqa  xmm6, [rsp+60h]
    movdqa  xmm7, [rsp+70h]
    movdqa  xmm8, [rsp+80h]
    movdqa  xmm9, [rsp+90h]
    movdqa  xmm10, [rsp+0A0h]
    movdqa  xmm11, [rsp+0B0h]
    movdqa  xmm12, [rsp+0C0h]
    movdqa  xmm13, [rsp+0D0h]
    movdqa  xmm14, [rsp+0E0h]
    movdqa  xmm15, [rsp+0F0h]
    add     rsp, 100h    ; 释放 XMM 寄存器空间
    
    ; 恢复通用寄存器（注意顺序与push相反）
    pop     rbp
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rbx
    pop     rdx
    pop     rcx
    pop     rax
    
    ; 跳转到 MinHook 提供的原始流程（trampoline）
    jmp     QWORD PTR [pRelWritePetPoint]

END

