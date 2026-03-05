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


extern GameVariables g_GameVars;

extern LPVOID pRelWritePetPoint;
extern LPVOID pRelWritePetPointRJ;
void GetPointByHeroID();

extern "C" void HookPetMove();

void MOutputDebugStringExA(const char* strOutputString, ...);
void MOutputDebugStringExW(const wchar_t* strOutputString, ...);
void RegHotKey();
void StartDrawWindow();
DWORD GetModuleLen(HMODULE hModule);
DWORD64 ScanPattern(const char* markCode, int nOffset, LPCSTR handle=NULL);

bool  InitCRC();
//void  HookPetMoveRJ();