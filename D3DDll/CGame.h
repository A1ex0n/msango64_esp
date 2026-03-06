#pragma once
#include <windows.h>
#define MSANGO_DEBUG


namespace msango
{
    enum hero
    {
        GameBase,       //地址
        CommonECX,      //通用ECX
        PetMove,        //pet移动
        HookAddr,       //hook地址
        HookAddrRJ,     //hook地址人机
        GetPosZ,        //获取z坐标
      
    };
    typedef struct _STRUCT_PATTERN_
    {
        ULONG32 enu;
        const char* pattern;
        const char* moduleName;
        DWORD64 offset;
        BOOL isptr;
        const char* description;
    } STRUCT_PARTTERN, * PSTRUCT_PARTTERN;

class CGame
{
public:
    static CGame& getInstance() {
        static CGame obj;
        return obj;
    }
    bool Init();

    bool BypassShop();
   // bool BypassVisualRange();
  //  bool BypassSnapshot();
    bool HookPet();

private:
    bool InitBase();
    bool InitHook();
public:
    ULONG64 m_addr[GetPosZ +1];
};
}
