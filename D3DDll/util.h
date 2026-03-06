#pragma once
#include "CGame.h"
#include <stdio.h>
#include <string>
#include "MemoryManager.h"
struct vec2
{
    float x;
    float y;
};
struct vec3
{
    float x;
    float y;
    float z;
};
struct vec4
{
    float x;
    float y;
    float z;
    float w;
};
struct GameVariables
{
    bool isGet;    
    bool isSus;     
    bool isWu;      
    int  TarIndex; 
    int  selfIndex; 

};

struct HotKeyInfo {
    int id;
    UINT mods;
    UINT vk;
};


// These globals are referenced from MASM (.asm). MASM expects undecorated names,
// so we must give them C linkage to avoid C++ name mangling.
#ifdef __cplusplus
extern "C" {
#endif

extern GameVariables g_GameVars;
extern vec2 g_HeroWorldPoint;
extern DWORD64 g_CurrrentObject;
extern DWORD64 g_SelfHeroObject;

extern vec2 g_homewu;
extern vec2 g_homewei;

extern  DWORD64 pRelWritePetPoint;
extern  DWORD64 pRelWritePetPointRJ;

#ifdef __cplusplus
}
#endif
void GetPointByHeroID();

extern "C" void HookPetMove();
extern "C" void HookPetMoveRJ();

void MOutputDebugStringExA(const char* strOutputString, ...);
void MOutputDebugStringExW(const wchar_t* strOutputString, ...);
void RegHotKey();
void StartDrawWindow();
DWORD GetModuleLen(HMODULE hModule);
DWORD64 ScanPattern(const char* markCode, DWORD64 nOffset, LPCSTR handle=NULL);

bool  InitCRC();