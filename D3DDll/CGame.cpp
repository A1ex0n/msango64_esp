#include "CGame.h"
#include "util.h"
#include "MemoryManager.h"
#include "xorstr.hpp"
#include "./minhook/include/MinHook.h"
// Match MinHook library to platform + CRT (Debug vs Release) to avoid MSVCRT conflicts.

#if defined(_DEBUG)
#pragma comment(lib,"./minhook/lib/libMinHook-x64-v141-mdd.lib")
#else
#pragma comment(lib,"./minhook/lib/libMinHook-x64-v141-md.lib")
#endif

bool msango::CGame::Init()
{
    if (!InitBase())
    {
        return false;
    }
    if (!InitHook())
    {
        return false;
    }
    return true;
}
bool msango::CGame::InitHook()
{
    if (MH_Initialize() != MH_OK)
    {
        MOutputDebugStringExA("[MSG] MH_Initialize  ERROR");
        return false;
    }

    return true;
}
bool msango::CGame::InitBase()
{


    STRUCT_PARTTERN msgPartterns[]//20250424
    {
        {GameBase,   xorstr_("74 ?? 45 33 C9 45 33 C0 8B D7 48 8B CE E8") ,0  ,-0x30,TRUE ,xorstr_("base")},//qword_7284FF0
        {CommonECX,  xorstr_("4D 85 C0 74 ?? 48 8B E9 49 2B E8 48 C1 FD 02") ,0 , 0x17,TRUE ,xorstr_("ecx")},//unk_727E9E0
        {PetMove,    xorstr_("8B ?? ?? ?? 45 33 DB 25 32 FF FF FF") ,0,-0x9F,FALSE,xorstr_("move call")},//sub_1BB8BF0
        {HookAddr,   xorstr_("83 A3 ?? ?? ?? ?? ?? E9 ?? ?? ?? ?? 85 C0 74 07") ,0, 0x1E,FALSE,xorstr_("hook pet posPoint")},//0000000001C2C3C0  rsi+0x110 rsi+0x110+4 
        {HookAddrRJ, xorstr_("F3 0F 11 88 ?? ?? ?? ?? 48 8B 47 10") ,0, 0/*无偏移*/,FALSE,xorstr_("hook pet posPointRJ")},//0000000004CAF4E9  rdi+0x2C  rdi+0x2C+4
        {GetPosZ,    xorstr_("F3 0F 2C C4 99 83 E2 7F 44 8D 0C 02 F3 0F") ,0      ,-0x51,FALSE,xorstr_("Get posZ func")},//sub_4B6AC50
    };
    for (ULONG32 i = 0; i < GetPosZ + 1; i++)
    {
        m_addr[i] = ScanPattern(msgPartterns[i].pattern, msgPartterns[i].offset, msgPartterns[i].moduleName);
        MOutputDebugStringExA("[MSG]  m_addr %s:  %llX\r", msgPartterns[i].description, m_addr[i]);
        if (msgPartterns[i].isptr)
        {
            CMemoryManager memman;
            DWORD offset = memman.RVM<DWORD>(m_addr[i]);// 70 42 5D 05       
            m_addr[i] = m_addr[i] + 4 + offset;

        }

        MOutputDebugStringExA("[MSG]  %s:  %llX\r", msgPartterns[i].description, m_addr[i]);

    }

    return true;
}


bool msango::CGame::HookPet()
{
    if (MH_CreateHook((LPVOID)m_addr[msango::HookAddr], &HookPetMove, (LPVOID*)&pRelWritePetPoint) != MH_OK)
    {
        MOutputDebugStringExA("[MSG] HookPetPointChange MH_CreateHook  ERROR");
        return false;
    }
    if (MH_CreateHook((LPVOID)m_addr[msango::HookAddrRJ], &HookPetMoveRJ, (LPVOID*)&pRelWritePetPointRJ) != MH_OK)
    {
        MOutputDebugStringExA("[MSG] HookPetPointChangeRJ MH_CreateHook  ERROR");
        return false;
    }
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        MOutputDebugStringExA("[MSG] StartHookPet MH_EnableHook  ERROR");
        return false;
    }
    return true;

}

