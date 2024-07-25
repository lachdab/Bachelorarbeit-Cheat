#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MINHOOK
#include "ext/MinHook/MinHook.h"
#pragma comment(lib, "ext/MinHook/libMinHook.x64.lib")

// IMGUI
#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_win32.h"
#include "ext/imgui/imgui_impl_dx11.h"

// OTHER
#include <d3d11.h>
#include <string>
#include "gui.h"
#include "misc.h"
#include <iostream>

WNDPROC oWndProc;
bool showImGuiMenu = true;
bool init = false;
HINSTANCE gui::dll_handle = nullptr;

HWND window = NULL;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;

// This is the prototype hook function for is the localPlayer shooting
uintptr_t moduleBase = (uintptr_t)GetModuleHandleW(L"GameAssembly.dll");
typedef bool(__fastcall* isShooting)(DWORD64* __this, DWORD64* methodInfo);
isShooting isShootingg;
isShooting isShootinggTarget = reinterpret_cast<isShooting>(moduleBase + 0x47d200);

bool __fastcall detourIsShooting(DWORD64*, DWORD64*)
{
    std::cout << "Shoot" << std::endl;
    return isShootingg(nullptr, nullptr);
}


// weitere hook function
/*typedef BOOL(WINAPI* peekMessageA)(LPMSG lpMsg, HWND  hWnd, UINT  wMsgFilterMin, UINT  wMsgFilterMax, UINT  wRemoveMsg);
peekMessageA pPeekMessageA = nullptr; //original function pointer after hook
peekMessageA pPeekMessageATarget; //original function pointer BEFORE hook do not call this!
BOOL WINAPI detourPeekMessageA(LPMSG lpMsg, HWND  hWnd, UINT  wMsgFilterMin, UINT  wMsgFilterMax, UINT  wRemoveMsg) {
    if (lpMsg->message == WM_KEYDOWN) {
        std::cout << "Key pressed" << std::endl;
    }
    return pPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}*/


//// Hooking drawIndex from DX11
//typedef void(__stdcall* ID3D11DrawIndexed)(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
//bool bDrawIndexed = true;
//ID3D11DrawIndexed fnID3D11DrawIndexed;
//ID3D11DrawIndexed fnID3D11DrawIndexed_target;
//// nicht sicher ob nötig
//DWORD_PTR* pDeviceContextVTable = NULL;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;


bool gui::get_present_pointer()
{
    std::wstring processName = L"Bachelorarbeit2.0";

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = gui::getHandlerByWindowTitle(processName);
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swap_chain;
    ID3D11Device* device;

    const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    // creation of the dx11 device and swap chain
    if (D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        feature_levels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        nullptr,
        nullptr) == S_OK)
    {
        void** p_vtable = *reinterpret_cast<void***>(swap_chain);
        /*pDeviceContextVTable = (DWORD_PTR*)p_vtable[0];
        fnID3D11DrawIndexed_target = (ID3D11DrawIndexed)p_vtable[12];
        std::cout << "[+] pDeviceContextVTable Addr: " << std::hex << pDeviceContextVTable << std::endl;
        std::cout << "[+] fnID3D11DrawIndexed Addr: " << std::hex << fnID3D11DrawIndexed_target << std::endl;*/
        swap_chain->Release();
        device->Release();
        //context->Release();
        p_present_target = (present)p_vtable[8];
        return true;
    }
    return false;
}

static long __stdcall gui::detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
    if (!init) {
        if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device)))
        {
            p_device->GetImmediateContext(&p_context);
            DXGI_SWAP_CHAIN_DESC sd;
            p_swap_chain->GetDesc(&sd);
            ImGui::CreateContext();
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)gui::WndProc);
            gui::ImGuiCustomStyle();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(p_device, p_context);
            init = true;
        }
        else
            return p_present(p_swap_chain, sync_interval, flags);
    }
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();

    ImGui::NewFrame();

    if (showImGuiMenu) {
        ImGui::Begin("Cheat Menu");
        ImGui::Text("with F1 show/hide cheat menu");

        if (ImGui::TreeNode("ESP"))
        {
            if (ImGui::TreeNode("Players"))
            {
                // Players section content
                static bool enableESP = false;
                ImGui::Checkbox("Enable ESP", &enableESP);
                if (enableESP)
                    // TODO: run the function to show player esp
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Weapons"))
            {
                // Weapons section content
                static bool espVehicles = false;
                ImGui::Checkbox("Show Weapons", &espVehicles);
                if (espVehicles)
                    // TODO: run the function to show weapon esp
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Aimbot"))
        {
            static bool enableAimbot = false;
            static float aimbotFOV = 0.5f;
            ImGui::Checkbox("Enable Aimbot", &enableAimbot);
            ImGui::SliderFloat("FOV", &aimbotFOV, 0.0f, 1.0f, "%.1f");

            if (ImGui::TreeNode("AI Aimbot"))
            {
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Misc"))
        {
            static bool playerIsShooting = false;
            static bool enableInfiniteAmmo = false;
            ImGui::Checkbox("Show if Player is Shooting", &playerIsShooting);
            ImGui::Checkbox("Enable Infinite Ammo", &enableInfiniteAmmo);

            if (playerIsShooting)
            {
                std::cout << "moduleBase: " << moduleBase << std::endl;
                std::cout << "isShootingg: " << isShootingg << std::endl;
                std::cout << "isShootinggTarget: " << isShootinggTarget << std::endl;
                std::cout << "Trying to enable the hook" << std::endl;
                MH_STATUS ttstatus = MH_EnableHook(isShootinggTarget);
                if (ttstatus != MH_OK)
                {
                    std::string ssStatus = MH_StatusToString(ttstatus);
                    std::cout << ssStatus << std::endl;
                    return 1;
                }
                std::cout << "Hook enabled" << std::endl;
            }
            if (enableInfiniteAmmo)
            {

            }
            ImGui::TreePop();
        }
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();

    p_context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return p_present(p_swap_chain, sync_interval, flags);
}

DWORD __stdcall gui::EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeConsole();
    FreeLibraryAndExitThread(gui::dll_handle, 0);
    Sleep(100);
    return 0;
}

bool AreHooksIntact()
{
    // Check if hooks are still valid
    return (MH_EnableHook(MH_ALL_HOOKS) == MH_OK);
}


// main code
int WINAPI gui::RunGUI()
{   
    if (!gui::get_present_pointer())
    {
        return 1;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        return 1;
    }

    if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &gui::detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
        return 1;
    }
    if (MH_EnableHook(p_present_target) != MH_OK) {
        return 1;
    }

    // TODO: create and enable hook for drawIndex from DX11
    /*MH_STATUS testStatusss = MH_CreateHook(reinterpret_cast<void**>(fnID3D11DrawIndexed_target), &hookD3D11DrawIndexed, reinterpret_cast<void**>(&fnID3D11DrawIndexed));
    if (testStatusss != MH_OK)
    {
        std::string ssStatus = MH_StatusToString(testStatusss);
        std::cout << ssStatus << std::endl;
        return 1;
    }
    if (MH_EnableHook(fnID3D11DrawIndexed_target) != MH_OK)
    {
        return 1;
    }*/

    //if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, reinterpret_cast<void**>(&pPeekMessageA), reinterpret_cast<void**>(&pPeekMessageATarget)) != MH_OK) {
    //    return 1;
    //}
    
    //std::cout << "Trying to create the hook" << std::endl;
    MH_STATUS testStatus = MH_CreateHook(isShootinggTarget, &detourIsShooting, reinterpret_cast<LPVOID*>(&isShootingg));
    if (testStatus != MH_OK)
    {
        std::string sStatus = MH_StatusToString(testStatus);
        std::cout << sStatus << std::endl;
        return 1;
    }

    while (true) {
        Sleep(2000);
        bool test = AreHooksIntact();
        std::cout << "Hooks Intact: " << test << std::endl;
    }

    // cleanup
    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
        return 1;
    }
    if (MH_Uninitialize() != MH_OK) {
        return 1;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
    if (p_context) { p_context->Release(); p_context = NULL; }
    if (p_device) { p_device->Release(); p_device = NULL; }
    SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));

    CreateThread(0, 0, gui::EjectThread, 0, 0, 0);

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp  IMGUI_IMPL_API
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall gui::WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ImGuiIO& io = ImGui::GetIO();
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(window, &cursorPos);
    io.MousePos = ImVec2((float)cursorPos.x, (float)cursorPos.y);

    if (uMsg == WM_KEYUP)
    {
        if (wParam == VK_F1)
        {
            showImGuiMenu = !showImGuiMenu;
        }
    }

    if (showImGuiMenu)
    {
        io.MouseDrawCursor = true;
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }
    else {
        io.MouseDrawCursor = false;
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HWND gui::getHandlerByWindowTitle(const std::wstring& windowTitle) {
    HWND window = FindWindowW(NULL, windowTitle.c_str());
    if (window != NULL) {
        DWORD processId;
        DWORD threadId = GetWindowThreadProcessId(window, &processId);
        if (threadId != 0) {
            return window;
        }
        else {
            return window;
        }
    }
    else {
        return window;
    }
    return window;
}

void gui::ImGuiCustomStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(15, 15);
    style.WindowRounding = 5.0f;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(12, 8);
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    
    style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
}
