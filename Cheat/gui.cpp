#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <Windows.h>
#include "gui.h"
#include <thread>

HWND hwnd = NULL;
WNDPROC oWndProc;
// Data
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Main code
int gui::RunGUI()
{   
    /* INFO: aktuell wird ein 2tes dx11 Device etc... erstellt was zu einem Rendering Problem vom Spiel verursacht (black background mit flackern)
    *  Code umschreiben:
    *  - Kein Dx11 Device, Context und SwapChain erstellen, sondern die vom Spiel holen und dann es in ImGui senden
    *  - Danach die ganzen Sachen freigeben und es wieder an das Spiel senden
    */
    std::wstring processName = L"Bachelorarbeit";

    hwnd = gui::getHandlerByWindowTitle(processName);
    if (!hwnd) return -1;

    gui::CreateDeviceD3D(hwnd);
    CreateRenderTarget();

    // Our state
    //ImVec4 clear_color = ImVec4(10.0f, 0.0f, 5.0f, 255.0f);

    // Setup Dear ImGui context
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    const float clear_color[] = { 0.0f, 0.0f, 0.0f, 255.0f };

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    bool showImGui = false;
    while (!done)
    {
        if (done)
            break;

        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_F1))
            showImGui = !showImGui;

        if (showImGui)
        {
            // here comes the cheat menu gui and function calls for the cheat features
            ImGui::ShowDemoWindow();
        }

        // Rendering
        ImGui::Render();
               
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);

        // Render ImGui if it should be shown
        if (showImGui)
        {
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            
        }
        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    return 0;
}

// Helper functions
bool gui::CreateDeviceD3D(HWND wHandler)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = wHandler;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    return true;
}

void gui::CleanupDeviceD3D()
{
    gui::CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void gui::CreateRenderTarget()
{   
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void gui::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI gui::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
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

// Function to convert HRESULT error code to string
std::string gui::GetErrorMessage(HRESULT hr) {
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);
    if (size == 0) {
        return "Failed to retrieve error message.";
    }
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

