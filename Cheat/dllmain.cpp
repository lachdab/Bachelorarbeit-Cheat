// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include <Windows.h>
#include <thread>
#include "gui.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // here comes the call for the cheat code and the menu with imgui
        //gui::RunGUI();
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gui::RunGUI, NULL, 0, NULL);

        /*gui::CreateHWindow("Cheat Menu", "Cheat Menu Class");
        gui::CreateDevice();
        gui::CreateImGui();

        // run gui loop
        while (gui::exit)
        {
            gui::BeginRender();
            gui::Render();
            gui::EndRender();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // destroy gui to free the memory
        gui::DestroyImGui();
        gui::DestroyDevice();
        gui::DestroyHWindow();
        */
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

