#include <windows.h> 

//#define _ATL_MIN_CRT
//#define _ATL_NO_FORCE_LIBS

int main()
{

}

#pragma comment(linker, "/export:CleanupA=_CleanupA@16")

extern "C" __declspec(dllexport) void CALLBACK CleanupA(HWND, HINSTANCE, PSTR, int) 
//int main()
{ 
    static MEMORY_BASIC_INFORMATION mbi; 
    VirtualQuery(&mbi, &mbi, sizeof mbi);
 
    PVOID module = mbi.AllocationBase;
    CHAR buf[MAX_PATH]; 

    GetModuleFileName(HMODULE(module), buf, sizeof buf);

     __asm 
    { 
        lea     eax, buf 
        push    0 
        push    0 
        push    eax 
        push    ExitProcess 
        push    module 
        push    DeleteFile 
        push    FreeLibrary 
        ret 
    }
}

