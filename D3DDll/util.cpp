#include <vector>
#include <string>
#include <d3d9.h>
#include <locale>
#include <codecvt>
#include "util.h"
#include "dxhook/dxhook.h"
#include "./draw/draw.h"
#include "./draw/imgui/imgui.h"
#include "./draw/imgui_impl_dx9.h"
#include "./draw/imgui_impl_win32.h"

//全局控制
GameVariables g_GameVars = { false, false, true, 0 , false};
vec2 g_HeroWorldPoint = { 0 };
std::string g_EnemyHeroName[5] = { "null","null", "null", "null", "null" };
//计算坐标（x64 下使用 64 位地址保存对象指针）
DWORD64 g_CurrrentObject = NULL;
DWORD64 g_SelfHeroObject = NULL;

/*
*******************功能*******************
*******************功能*******************
*******************功能*******************
*******************功能*******************
*******************功能*******************
*******************功能*******************
*/
//编码转换
std::string gbk2utf8(std::string gbk)
{

    //GBK locale name in windows
    const char* const GBK_LOCALE_NAME = ".936";

    // Create a GBK codecvt facet
    std::wstring_convert<std::codecvt_byname<wchar_t, char, std::mbstate_t>>
        gbkConverter(new std::codecvt_byname<wchar_t, char, std::mbstate_t>(GBK_LOCALE_NAME));
    // Convert GBK string to wstring
    std::wstring wstr = gbkConverter.from_bytes(gbk);
    // Create a UTF-8 codecvt facet
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8Converter;
    // Convert wstring to UTF-8 string
    return utf8Converter.to_bytes(wstr);
}
//调试字符串
void MOutputDebugStringExW(const wchar_t* strOutputString, ...)
{
    va_list vlArgs = NULL;
    va_start(vlArgs, strOutputString);
    size_t nLen = _vscwprintf(strOutputString, vlArgs) + 1;
    wchar_t* strBuffer = new wchar_t[nLen];
    _vsnwprintf_s(strBuffer, nLen, nLen, strOutputString, vlArgs);
    va_end(vlArgs);
    OutputDebugStringW(strBuffer);
    delete[] strBuffer;
}
void MOutputDebugStringExA(const char* strOutputString, ...)
{
    va_list vlArgs = NULL;
    va_start(vlArgs, strOutputString);
    size_t nLen = _vscprintf(strOutputString, vlArgs) + 1;
    char* strBuffer = new char[nLen];
    _vsnprintf_s(strBuffer, nLen, nLen, strOutputString, vlArgs);
    va_end(vlArgs);
    OutputDebugStringA(strBuffer);
    delete[] strBuffer;
}
//获取模块大小
DWORD GetModuleLen(HMODULE hModule)
{
    PBYTE pImage = (PBYTE)hModule;
    PIMAGE_DOS_HEADER pImageDosHeader;
    PIMAGE_NT_HEADERS pImageNtHeader;
    pImageDosHeader = (PIMAGE_DOS_HEADER)pImage;
    if (pImageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return 0;
    }
    pImageNtHeader = (PIMAGE_NT_HEADERS)&pImage[pImageDosHeader->e_lfanew];
    if (pImageNtHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        return 0;
    }
    return pImageNtHeader->OptionalHeader.SizeOfImage;
}
//搜索特征码

 DWORD64 ScanPattern(const char* markCode, int nOffset, LPCSTR handle)
{
    DWORD64 StartAddr = (DWORD64)GetModuleHandleA(handle);
    DWORD64 unMoudlelenth = GetModuleLen((HMODULE)StartAddr);
    std::string input = markCode;
    std::string output;
    for (char c : input) {
        if (c != ' ') {
            output += c;
        }
    }
    StartAddr += 0x1000;
    BYTE* pScanf = (BYTE*)StartAddr;
    //************处理特征码，转化成字节*****************
    if (output.length() % 2 != 0)
    {
        //不符合长度要求
        return 0;
    }
    //特征码长度
    size_t len = output.length() / 2;  //获取代码的字节数
    //将特征码转换成byte型 保存在m_code 中
    BYTE* m_code = new BYTE[len];
    char* m_Flag = new char[len];
    int nFlagCount = 0;
    RtlZeroMemory(m_code, len);
    RtlZeroMemory(m_Flag, len);
    for (int i = 0; i < len; i++)
    {
        //通配符
        if (output[i * 2] == '?')
        {
            m_Flag[i] = '?';
            nFlagCount++;
        }
        else
        {
            //定义可容纳单个字符的一种基本数据类型。
            char c[] = { output[i * 2], output[i * 2 + 1], '\0' };
            //将参数nptr字符串根据参数base来转换成长整型数
            m_code[i] = (BYTE)::strtol(c, NULL, 16);
        }
    }
    /////////////////////////查找特征码/////////////////////
    int nCmp;
    for (unsigned int i = 0; i < unMoudlelenth; i++)
    {
        nCmp = 0;
        if (m_code[0] == pScanf[i])//与第一个字节相同，首字节肯定不是通配符
        {
            nCmp++;
            for (int j = 0; j < len - 1; j++)
            {
                if (m_Flag[j + 1] == '?')//处理通配符
                {
                    nCmp++;
                    continue;
                }
                if (m_code[j + 1] == pScanf[i + j + 1])//比较每一个字节的大小，不相同则退出
                {
                    nCmp++;
                }
                else
                {
                    nCmp = 0;
                    break;
                }
            }
            if (nCmp == len)
            {
                // 找到特征码处理
                DWORD64 ullRet = (DWORD64)pScanf + i + nOffset;
                return ullRet;
            }
        }
    }
    return 0;
}
/*
*******************核心*******************
*******************核心*******************
*******************核心*******************
*******************核心*******************
*******************核心*******************
*******************核心*******************
*/
//获取xy坐标
vec2 g_homewu  ={ -6937.0f, -7037.0f};
vec2 g_homewei ={  6555.0f,  6333.0f};

// ASM 文件中实现的跟随函数
extern "C" void __stdcall followEnemyAsm(DWORD* pPetID, DWORD dwEnemyID, DWORD uECX, DWORD pPetMove);

void gohome(DWORD *pPetID)
{
    //ULONG64 _RCX = msango::CGame::getInstance().m_addr[msango::CommonECX];
    //ULONG64 pPetMove = msango::CGame::getInstance().m_addr[msango::PetMove];

    //__asm
    //{
    //    push 00000000h
    //    push 00000000h    //    大于 -6937
    //    mov eax, 1
    //    cmp al, [g_GameVars.isWu]
    //    jnz wei
    //    push g_homewu.y 
    //    push g_homewu.x 
    //    jmp work
    //wei :
    //    push g_homewei.y
    //    push g_homewei.x
    //work :
    //    push pPetID
    //    push 00000001h
    //    mov ecx, [uECX]
    //    mov eax, [pPetMove]
    //    call eax
    //}
    //return;
}

void followEnemy(DWORD *pPetID, DWORD dwEnemyID)
{
    // 从 C++ 里取出 ecx 和 函数地址，实际调用逻辑放到 ASM 里
    ULONG32 uECX    = msango::CGame::getInstance().m_addr[msango::CommonECX];
    ULONG32 pPetMove = msango::CGame::getInstance().m_addr[msango::PetMove];

    followEnemyAsm(pPetID, dwEnemyID, uECX, pPetMove);
}
//核心函数
void GetPointByHeroID()
{
    CMemoryManager memGet;
    while (1)
    {
        Sleep(1000);
        if (g_GameVars.isGet)
        {
            BOOL isZeroAddr = FALSE;
            DWORD HeroID[5] = { 0 };
            DWORD64 dwObj = NULL;
            DWORD EnemyIndex = g_GameVars.isWu ? 5 : 0;
            DWORD64 dwBaseAddress = memGet.RVM<DWORD64>(msango::CGame::getInstance().m_addr[msango::GameBase]);
            if (NULL == dwBaseAddress) 
            {
                MOutputDebugStringExA("[MSG] BaseAddress is nullptr");
                g_GameVars.isGet = false;
                continue;
            }
   /*       dump [[[[[[7284FF0]+b3b0]+0x30]+0x10+5d*8]+0x20]+0x78]
            [gamebase] + 0xB3B0] + 0x30] + ID * 8 + 0x10] + 0x20     单位对象
            [gamebase] + 0xB3B0] + 0x30] + ID * 8 + 0x10] + 0x20] + 0x18     单位对象ID
            [gamebase] + 0xB3B0] + 0x30] + ID * 8 + 0x10] + 0x20] + 0x78     单位对象属性
            [gamebase] + 0xB3B0] + 0x30] + ID * 8 + 0x10] + 0x20] + 0x78] + 0xb8     单位对象name*/
            for (size_t i = 0; i < 5; i++)
            {
                HeroID[i] = 0;
                g_EnemyHeroName[i] = "NULL";
                dwObj = memGet.RVM<DWORD64>(dwBaseAddress + 0xB3B0);//当前对局全体对象数组，以ID排列，ID自增
                dwObj = memGet.RVM<DWORD64>(dwObj + (0x5D+ EnemyIndex * 2) *8 + 0x10);//对象指针
                dwObj = memGet.RVM<DWORD64>(dwObj + 0x20);//对象属性指针
                if (dwObj == 0) 
                {
                    isZeroAddr = TRUE;
                    break;
                }
                HeroID[i] = memGet.RVM<DWORD>(dwObj + 0x18);//id
                dwObj = memGet.RVM<DWORD64>(dwObj + 0x78);//属性
                g_EnemyHeroName[i] = (char*)(dwObj + 0xb8);//name
                if (HeroID[i] == 0) 
                {
                    isZeroAddr = TRUE;
                    break;
                }
                MOutputDebugStringExA("[MSG] %s:%x\r", g_EnemyHeroName[i].c_str(), HeroID[i]);
            }
            if (isZeroAddr) 
            {
                MOutputDebugStringExA("[MSG] get name&ID err....");
                g_GameVars.isGet = false;
                continue;
            }
            // 
            DWORD dwSelfID = 0x5D+ g_GameVars.selfIndex*2;
            DWORD dwPetID = dwSelfID + 1;
            DWORD *pPetID = new DWORD ;
            *pPetID = dwPetID;
            MOutputDebugStringExA("[MSG] selfID:%x ,petID:%x", dwSelfID, dwPetID);

            //获取全体对象数组
            DWORD tmp = memGet.RVM<DWORD64>(dwBaseAddress + 0xB3B0);
            tmp = memGet.RVM<DWORD64>(tmp + 0x30);
            //获取自身对象，存起来获取z坐标用
            DWORD tmpHero = memGet.RVM<DWORD64>(tmp + dwSelfID * 8 + 0x10);
            g_SelfHeroObject = memGet.RVM<DWORD64>(tmpHero + 0x20);
            //获取自身pet对象， 存起来获取xy坐标用
            DWORD tmpPetObj = memGet.RVM<DWORD64>(tmp + dwPetID * 8 + 0x10);
            g_CurrrentObject = memGet.RVM<DWORD64>(tmpPetObj + 0x20);
            //判断是否可用
            if (0 == dwPetID || 0 == g_SelfHeroObject) 
            {
                g_CurrrentObject = NULL;
                g_GameVars.isGet = false;
                continue;
            }
            //work
            while (1)
            {
                if (!g_GameVars.isGet) 
                {
                    g_CurrrentObject = NULL;
                    g_SelfHeroObject = NULL;
                    delete pPetID;
                    break;
                }
                if (!g_GameVars.isSus)
                {
                    Sleep(100);
                    continue;
                }
                Sleep(50);
                followEnemy(pPetID, HeroID[g_GameVars.TarIndex]);
                Sleep(50);
                gohome(pPetID);
            }
        }
    }
}
/*
*******************绘制*******************
*******************绘制*******************
*******************绘制*******************
*******************绘制*******************
*******************绘制*******************
*******************绘制*******************
*/
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oWndProc;
EndScene oEndScene = NULL;
//小地图偏移
float g_offX = 777.0;
float g_offY = 159.0;
//大地图偏移
float g_BigMapoff = 115.0f;
//目标窗口，绘制在此窗口上层
HWND g_hTarWnd = FindWindowA(NULL, "梦三国2 Online");
//HWND g_hTarWnd = FindWindowA(NULL, "D3D Tutorial");
//世界坐标转屏幕坐标
DWORD64 g_pEcx = (DWORD64)GetProcAddress(GetModuleHandleA("Engine.dll"), "?g_pkGraphic@@3PEAVIGraph@@EA");//
DWORD64 g_pFun = (DWORD64)GetProcAddress(GetModuleHandleA("Engine.dll"), "?GetScreenPosition@IGraph@@QEAAXAEBUD3DXVECTOR3@@AEAUD3DXVECTOR2@@@Z");
//小地图坐标转换
vec2 TransMinimap(vec2 strWorld)//小地图边框厚度也有影响
{
    vec2 d3dRet;
    LONG Height;
    RECT rect;
    GetWindowRect(g_hTarWnd, &rect);
    Height = rect.bottom - rect.top;
    d3dRet.x = (float)(172.0 / 17900.0 * strWorld.x + 78.0 + g_offX);
    d3dRet.y = (float)(166.0 - 166.0 / 17300.0 * strWorld.y - 91.0 + Height - g_offY);
    return(d3dRet);
}
//call内部函数，获取z坐标
float GetZ(DWORD myecx, float x, float y)
{
    ////fstp dword ptr ss:[esp+0x1C], st0
    float res = 0.0;
    //DWORD pGetPos = msango::CGame::getInstance().m_addr[msango::GetPosZ];
    //__asm
    //{
    //    push eax
    //    mov eax, [pGetPos]
    //    push y
    //    push x
    //    mov ecx, [myecx]
    //    call eax
    //    pop eax
    //    fstp[res]
    //}
    return res;
}
//call内部函数，转换屏幕坐标
BOOL WorldToScreen(vec3& pos, vec2& screen)
{
    //CMemoryManager mem;
    //DWORD dwEcx = mem.RVM<DWORD>(g_pEcx);
    DWORD res = 0;
    //__asm
    //{
    //    mov ecx, [dwEcx]
    //    push screen
    //    push pos
    //    mov eax, g_pFun
    //    call eax
    //    mov[res], eax
    //}

    return res;
}
//计算大地图偏移
vec2 calculateThirdPoint(const vec2& pHome, const vec2& point2, float distance)
{
    float dx = point2.x - pHome.x;
    float dy = point2.y - pHome.y;
    float d = std::sqrt(dx * dx + dy * dy);

    float ratio = distance / d;
    float x3 = point2.x + ratio * dx;
    float y3 = point2.y + ratio * dy;
    vec2 retvec2 = { x3,y3 };
    return retvec2;
}
//绘制函数
void DrawFun()
{
    std::vector<std::string> infoStrings;
    infoStrings.emplace_back(gbk2utf8("临时: (" + std::string(g_GameVars.isSus ? "开启)" : "关闭)") + "     alt+1"));

    infoStrings.emplace_back(gbk2utf8("英雄: (" + std::string(std::to_string(g_GameVars.TarIndex) 
        + " "+ g_EnemyHeroName[g_GameVars.TarIndex]) + ")"+"   alt+2"));

    infoStrings.emplace_back(gbk2utf8("自身: (" + std::string(std::to_string(g_GameVars.selfIndex))
        +" "+std::string(g_GameVars.isWu ? "吴)" : "魏)") + "       alt+4"));

    infoStrings.emplace_back(gbk2utf8("对局: (" + std::string(g_GameVars.isGet ? "开启)" : "关闭)") + "     alt+HOME"));
    for (size_t i = 0; i < infoStrings.size(); ++i)
    {
        Draw_Text(150, float(300 + i * 35),infoStrings[i].c_str(),ImColor(255,0,0));
    }

    //CMemoryManager mem;
    //if (g_GameVars.isGet && g_GameVars.isSus)
    //{
    //    vec2 tmpD3D = { 0 };
    //    tmpD3D = TransMinimap(g_HeroWorldPoint);
    //    //小地图
    //    DrawCircle(tmpD3D.x, tmpD3D.y,6.2f, ImColor(0, 255, 0));
    //    //大地图
    //    DWORD tmpEcx = mem.RVM<DWORD>(g_SelfHeroObject + 0X10);//0X10
    //    if (tmpEcx){
    //        tmpEcx = mem.RVM<DWORD>(tmpEcx + 0C930h);//0C930h
    //        if (tmpEcx){
    //            tmpEcx = mem.RVM<DWORD>(tmpEcx + 0X10);//0X10
    //            if (tmpEcx){
    //                vec3 worldV3 = { 0 };
    //                worldV3.x = g_HeroWorldPoint.x;
    //                worldV3.y = g_HeroWorldPoint.y;
    //                vec3 homev3 ;
    //                if (g_GameVars.isWu)
    //                {
    //                    homev3.x = g_homewu.x;
    //                    homev3.y = g_homewu.y;
    //                }
    //                else
    //                {
    //                    homev3.x = g_homewei.x;
    //                    homev3.y = g_homewei.y;
    //                }
    //                vec2 tmpv2= calculateThirdPoint({ homev3.x,homev3.y }, { worldV3.x, worldV3.y }, g_BigMapoff);
    //                vec3 relPointV3 = { 0 };
    //                relPointV3.x = tmpv2.x;
    //                relPointV3.y = tmpv2.y;
    //                relPointV3.z = GetZ(tmpEcx,relPointV3.x, relPointV3.y);
    //                vec2 screenV2 = { 0 };
    //                if (WorldToScreen(relPointV3, screenV2)){

    //                    DrawCircle(screenV2.x, screenV2.y,6.2f, ImColor(255,0,0));
    //                }                                               
    //            }
    //        }
    //    }
    //}
}
//开始绘制
int InitImgui(HWND hwnd, LPDIRECT3DDEVICE9 pDevice)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(pDevice);
    // Load Fonts

    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    static const ImWchar myGlyphRanges[] = {
        0x2588,0x2588,// █ 
        0x25cf,0x25cf,// ●
    0
    };
    builder.AddRanges(myGlyphRanges);
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
    builder.BuildRanges(&ranges);

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simhei.ttf", 18.0f, 0, ranges.Data);
    assert(font != NULL);
    io.Fonts->Build();

    return 0;
}
long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
    static bool g_bImguiInit = false;
    if (!g_bImguiInit)
    {
        InitImgui(g_hTarWnd,pDevice);
        g_bImguiInit = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawFun();
    ImGui::EndFrame();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    return oEndScene(pDevice);
}
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}
void StartDrawWindow()
{
    if (dx_init() == TRUE)
    {
        dx_bind(42, (void**)&oEndScene, hkEndScene);
    }
    oWndProc = (WNDPROC)SetWindowLongPtr(g_hTarWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}
/*
*******************热键*******************
*******************热键*******************
*******************热键*******************
*******************热键*******************
*******************热键*******************
*******************热键*******************
*/
// 定义热键数组
HotKeyInfo hotkeys[] = {
    {195, MOD_ALT | MOD_NOREPEAT, VK_HOME},
    {196, MOD_ALT | MOD_NOREPEAT, 0x31},
    {197, MOD_ALT | MOD_NOREPEAT, 0x32},
    {198, MOD_ALT | MOD_NOREPEAT, 0x34},
    {199, MOD_ALT | MOD_NOREPEAT, VK_INSERT},//调试使用
    {200, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD1},
    {201, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD2},
    {202, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD4},
    {203, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD5},
    {204, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD7},
    {205, MOD_ALT | MOD_NOREPEAT, VK_NUMPAD8},

};
// 定义处理热键消息的函数
void OnHotKey(int id) {
    switch (id) {
    case 195://home
        g_GameVars.isGet = !g_GameVars.isGet;
        break;
    case 196:
        g_GameVars.isSus = !g_GameVars.isSus;
        break;
    case 197:
        g_GameVars.TarIndex = (g_GameVars.TarIndex + 1) % 5;
        break;
    case 198:
        g_GameVars.selfIndex = (g_GameVars.selfIndex + 1) % 10;
        g_GameVars.isWu = g_GameVars.selfIndex > 4 ? false : true;
        break;
    case 199:
        g_GameVars.isWu =!g_GameVars.isWu;
        break;
    case 200:
        g_offX++;
        break;
    case 201:
        g_offX--;
        break;
    case 202:
        g_offY++;
        break;
    case 203:
        g_offY--;
        break;
    case 204:
        g_BigMapoff+=3;
        break;
    case 205:
        g_BigMapoff-=3;

        break;
    default:
        break;
    }
}
// 修改 RegHotKey 函数的实现
void RegHotKey() 
{
    MSG msg;
    for (int i = 0; i < sizeof(hotkeys) / sizeof(HotKeyInfo); i++) 
    {
        if (!RegisterHotKey(NULL, hotkeys[i].id, hotkeys[i].mods, hotkeys[i].vk)) 
        {
            char buf[128];
            sprintf_s(buf, "RegHotKey %d Error", i+1);
            MessageBoxA(NULL, buf, NULL, NULL);
        }
    }

    while (GetMessageA(&msg, NULL, 0, 0) != 0) 
    {
        if (msg.message == WM_HOTKEY) 
        {
            OnHotKey(msg.wParam);
        }
    }
}

/*
*******************HOOK相关*******************
*******************HOOK相关*******************
*******************HOOK相关******************* 
*******************HOOK相关*******************
*******************HOOK相关*******************
*******************HOOK相关*******************
*/
//偏移
#define MOVETO_OFFSET 0x27F0
//原流程
LPVOID pRelWritePetPoint;
LPVOID pRelWritePetPointRJ;
int  __stdcall cmpfloat(float x, float y)
{
    //mov ecx, 0x0C5CB20CC; f2
    if (g_GameVars.isWu)//是吴国
    {
        if (x > -6500.0f || y > -6500.0f)
        {
            return 0;
        }
        return -1;
    }
    else//是魏国
    {
        if (x < 6300.0f || y < 6300.0f)
        {
            return 0;
        }
        return -1;
    }

}
//void __declspec(naked) HookPetMove()
//{
//    __asm
//    {
//        pushad
//        mov eax, 1
//        cmp al, [g_GameVars.isGet]
//        jnz end
//        cmp ebx, [g_CurrrentObject]
//        jnz end
//        ; x / y有一符合则记录
//        xor eax, eax
//        xor ecx, ecx
//        mov eax, [ebx + g_moveoff]; x
//        mov ecx, [ebx + g_moveoff + 4]; y
//        push ebx
//        push ecx; y
//        push eax; x
//        call cmpfloat
//        pop ebx
//        test eax, eax
//        je record
//        jmp end
//    record:
//        mov eax, [ebx + g_moveoff]
//        mov[g_HeroWorldPoint.x], eax
//        mov eax, [ebx + g_moveoff + 4]
//        mov[g_HeroWorldPoint.y], eax
//    end :
//        popad
//        jmp pRelWritePetPoint
//    }
//}
//void __declspec(naked) HookPetMoveRJ()
//{
//    __asm
//    {
//        pushad
//        mov eax, 1
//        cmp al, [g_GameVars.isGet]
//        jnz end
//        cmp ecx, [g_CurrrentObject]
//        jnz end
//        ; x / y有一符合则记录
//        xor eax, eax
//        xor edx, edx
//        mov eax, [ecx + g_moveoff]; x
//        mov edx, [ecx + g_moveoff + 4]; y
//        push ecx
//        push edx;y
//        push eax;x
//        call cmpfloat
//        pop ecx
//        test eax, eax
//        jz record
//        jmp end
//    record :
//        mov eax, [ecx + g_moveoff]
//        mov[g_HeroWorldPoint.x], eax
//        mov eax, [ecx + g_moveoff + 4]
//        mov[g_HeroWorldPoint.y], eax
//    end :
//        popad
//        jmp pRelWritePetPointRJ
//    }
//}

