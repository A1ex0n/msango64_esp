
LONG64 dx_init();
void dx_shutdown();

LONG64 dx_bind(LONG64 index, void** original, void* function);
void dx_unbind(LONG64 index);
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);

LONG64* dx_getMethodsTable();
