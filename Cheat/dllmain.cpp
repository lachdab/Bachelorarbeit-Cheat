// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gui.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
		gui::dll_handle = hModule;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gui::RunGUI, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

