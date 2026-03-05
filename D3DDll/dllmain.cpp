#include <thread>
#include "util.h"
#include "common.h"
#include "xorstr.hpp"
//x64版本无需处理CRC 没启用
/*
    实现最小化的单人透视

    1.首先需要hookpet的“目的坐标”  ，把pet“目的坐标”绘制到屏幕上

    2.调用pet移动call，让其移动至某位hero处，飞行模式的pet的目的坐标，就是hero的坐标

    
*/
BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved )
{
    _Unreferenced_parameter_(hModule);
    _Unreferenced_parameter_(lpReserved);
    if (ul_reason_for_call==DLL_PROCESS_ATTACH )
    {
        
        if (!msango::CGame::getInstance().Init())//初始化minhook，初始化基址 
        {
            MessageBoxA(NULL, xorstr_("Init err"), NULL, NULL);
            return FALSE;
        }


        if (!msango::CGame::getInstance().HookPet()) {
            MessageBoxA(NULL, xorstr_("HookPet err"), NULL, NULL);
            return FALSE;
        }

  
        std::thread (RegHotKey).detach();
        std::thread (StartDrawWindow).detach();
        std::thread (GetPointByHeroID).detach();
        
  

    }
    return TRUE;
}
