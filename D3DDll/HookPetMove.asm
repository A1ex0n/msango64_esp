option casemap:none

EXTERN g_GameVars:BYTE
EXTERN g_CurrrentObject:QWORD
EXTERN g_HeroWorldPoint:QWORD
EXTERN pRelWritePetPoint:QWORD
EXTERN pRelWritePetPointRJ:QWORD

.const
flt_neg6500 REAL4 -6500.0        ; -6500.0f
flt_6300   REAL4  6300.0         ;  6300.0f

.code

PUBLIC HookPetMove
HookPetMove:
    ; 保存会用到的易失寄存器（GPR + XMM）
    push    rax
    push    rcx
    push    rdx

    sub     rsp, 48                     ; 为 xmm0/xmm1/xmm2 预留 3*16 字节
    movdqu  XMMWORD PTR [rsp], xmm0
    movdqu  XMMWORD PTR [rsp+16], xmm1
    movdqu  XMMWORD PTR [rsp+32], xmm2

    ; if (!g_GameVars.isGet) goto skip_logic;
    mov     al, BYTE PTR [g_GameVars]    ; GameVariables::isGet 位于结构体首字节
    test    al, al
    jz      skip_logic

    ; if (rbx != g_CurrrentObject) goto skip_logic;
    mov     rax, QWORD PTR [g_CurrrentObject]
    cmp     rbx, rax;rbx是当前单位的this指针
    jne     skip_logic

    ; 调用 cmpfloat 判断是否在老家角落
    ; x64 调用约定：浮点参数用 XMM0, XMM1
    ; 加载 x 坐标到 XMM0 (float)
    movss   xmm0, DWORD PTR [rsi+110h]

    ; 加载 y 坐标到 XMM1 (float)
    movss   xmm1, DWORD PTR [rsi+114h]

    ; 内联实现 cmpfloat 逻辑：
    ; if (g_GameVars.isWu) {
    ;     if (x > -6500.0f || y > -6500.0f)  // 不在老家角落
    ;         record;
    ;     else                               // 在老家角落
    ;         goto skip_logic;
    ; } else {
    ;     if (x < 6300.0f || y < 6300.0f)    // 不在老家角落
    ;         record;
    ;     else                               // 在老家角落
    ;         goto skip_logic;
    ; }

    ; 读取 g_GameVars.isWu
    mov     al, BYTE PTR [g_GameVars+2]
    test    al, al
    jz      is_wei

    ; ---------- 吴国逻辑 ----------
is_wu:
    movss   xmm2, DWORD PTR [flt_neg6500]
    comiss  xmm0, xmm2
    ja      record_coord          ; x > -6500.0f

    comiss  xmm1, xmm2
    ja      record_coord          ; y > -6500.0f

    jmp     skip_logic            ; x <= -6500 && y <= -6500 -> 在老家角落

    ; ---------- 魏国逻辑 ----------
is_wei:
    movss   xmm2, DWORD PTR [flt_6300]
    comiss  xmm0, xmm2
    jb      record_coord          ; x < 6300.0f

    comiss  xmm1, xmm2
    jb      record_coord          ; y < 6300.0f

    jmp     skip_logic            ; x >= 6300 && y >= 6300 -> 在老家角落

record_coord:
    ; 记录坐标：
    ; X = [rsi+110h]
    ; Y = [rsi+114h]
    mov     eax, DWORD PTR [rsi+110h]
    mov     DWORD PTR [g_HeroWorldPoint], eax       ; g_HeroWorldPoint.x

    mov     eax, DWORD PTR [rsi+114h]
    mov     DWORD PTR [g_HeroWorldPoint+4], eax     ; g_HeroWorldPoint.y

skip_logic:
    ; 还原 XMM 寄存器
    movdqu  xmm0, XMMWORD PTR [rsp]
    movdqu  xmm1, XMMWORD PTR [rsp+16]
    movdqu  xmm2, XMMWORD PTR [rsp+32]
    add     rsp, 48

    ; 还原 GPR
    pop     rdx
    pop     rcx
    pop     rax

    ; 跳转到 MinHook 提供的原始流程（trampoline）
    jmp     QWORD PTR [pRelWritePetPoint]

;-------------------------------------------------------------------------------
; HookPetMoveRJ
; RJ 情况：
;   - this 指针在 RAX
;   - 坐标在 [rdi+0x2C] (x), [rdi+0x2C+4] (y)
; 逻辑和 HookPetMove 完全一致，只是寄存器/偏移不同
;-------------------------------------------------------------------------------
PUBLIC HookPetMoveRJ
HookPetMoveRJ:
    ; 保存会用到的易失寄存器（GPR + XMM）
    push    rax
    push    rcx
    push    rdx

    sub     rsp, 48                     ; 为 xmm0/xmm1/xmm2 预留 3*16 字节
    movdqu  XMMWORD PTR [rsp], xmm0
    movdqu  XMMWORD PTR [rsp+16], xmm1
    movdqu  XMMWORD PTR [rsp+32], xmm2

    ; 额外为原始 this（RAX）预留 8 字节
    sub     rsp, 8
    mov     QWORD PTR [rsp], rax        ; 保存原始 this

    ; if (!g_GameVars.isGet) goto skip_logic_rj;
    mov     al, BYTE PTR [g_GameVars]
    test    al, al
    jz      skip_logic_rj

    ; if (RAX(this) != g_CurrrentObject) goto skip_logic_rj;
    mov     rcx, QWORD PTR [g_CurrrentObject]
    mov     rdx, QWORD PTR [rsp]          ; 栈顶保存的原始 RAX(this)
    cmp     rdx, rcx
    jne     skip_logic_rj

    ; 加载 x 坐标到 XMM0: [rdi+0x2C]
    movss   xmm0, DWORD PTR [rdi+2Ch]

    ; 加载 y 坐标到 XMM1: [rdi+0x2C+4]
    movss   xmm1, DWORD PTR [rdi+30h]

    ; 读取 g_GameVars.isWu
    mov     al, BYTE PTR [g_GameVars+2]
    test    al, al
    jz      is_wei_rj

    ; ---------- 吴国逻辑 ----------
is_wu_rj:
    movss   xmm2, DWORD PTR [flt_neg6500]
    comiss  xmm0, xmm2
    ja      record_coord_rj          ; x > -6500.0f

    comiss  xmm1, xmm2
    ja      record_coord_rj          ; y > -6500.0f

    jmp     skip_logic_rj            ; x <= -6500 && y <= -6500 -> 在老家角落

    ; ---------- 魏国逻辑 ----------
is_wei_rj:
    movss   xmm2, DWORD PTR [flt_6300]
    comiss  xmm0, xmm2
    jb      record_coord_rj          ; x < 6300.0f

    comiss  xmm1, xmm2
    jb      record_coord_rj          ; y < 6300.0f

    jmp     skip_logic_rj            ; x >= 6300 && y >= 6300 -> 在老家角落

record_coord_rj:
    ; X = [rdi+0x2C]
    ; Y = [rdi+0x2C+4]
    mov     eax, DWORD PTR [rdi+2Ch]
    mov     DWORD PTR [g_HeroWorldPoint], eax       ; g_HeroWorldPoint.x

    mov     eax, DWORD PTR [rdi+30h]
    mov     DWORD PTR [g_HeroWorldPoint+4], eax     ; g_HeroWorldPoint.y

skip_logic_rj:
    ; 还原原始 this 到 RAX
    mov     rax, QWORD PTR [rsp]
    add     rsp, 8

    ; 还原 XMM 寄存器
    movdqu  xmm0, XMMWORD PTR [rsp]
    movdqu  xmm1, XMMWORD PTR [rsp+16]
    movdqu  xmm2, XMMWORD PTR [rsp+32]
    add     rsp, 48

    ; 还原 GPR
    pop     rdx
    pop     rcx
    pop     rax

    ; 跳转到 RJ 版本的原始流程（trampoline）
    jmp     QWORD PTR [pRelWritePetPointRJ]

END

