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
#include <iostream>
#include <unordered_set>
#include <mutex>
#include <d3dcompiler.h>
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// Model Structures
struct propertiesModel
{
    UINT stride;
    UINT vedesc_ByteWidth;
    UINT indesc_ByteWidth;
    UINT pscdesc_ByteWidth;
};

static bool operator==(const propertiesModel& lhs, const propertiesModel& rhs)
{
    if (lhs.stride != rhs.stride
        || lhs.vedesc_ByteWidth != rhs.vedesc_ByteWidth
        || lhs.indesc_ByteWidth != rhs.indesc_ByteWidth
        || lhs.pscdesc_ByteWidth != rhs.pscdesc_ByteWidth)
    {
        return false;
    }
    else
    {
        return true;
    }
}

namespace std {
    template<> struct hash<propertiesModel>
    {
        std::size_t operator()(const propertiesModel& obj) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(obj.stride);
            std::size_t h2 = std::hash<int>{}(obj.vedesc_ByteWidth);
            std::size_t h3 = std::hash<int>{}(obj.indesc_ByteWidth);
            std::size_t h4 = std::hash<int>{}(obj.pscdesc_ByteWidth);
            return (h1 + h3 + h4) ^ (h2 << 1);
        }
    };
}

WNDPROC oWndProc;
bool showImGuiMenu = false;
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

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;

// Hooking DrawIndexedInstanced from DX11 for PlayerModel and Cases
typedef void(__stdcall* ID3D11DrawIndexedInstanced)(ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
ID3D11DrawIndexedInstanced fnID3D11DrawIndexedInstanced = nullptr;
ID3D11DrawIndexedInstanced fnID3D11DrawIndexedInstanced_target = nullptr;
bool drawIndexedInstanced = false;
bool firstTime = true;
DWORD_PTR* pDeviceContextVTable = NULL;

// Wallhack
// Texture for the shader
ID3D11Texture2D* textureRed = nullptr;
ID3D11ShaderResourceView* textureView;
ID3D11SamplerState* pSamplerState;
ID3D11PixelShader* pShaderRed = NULL;
ID3D11PixelShader* pShaderBlue = NULL;
bool bShader = false;
// Texture color for the shader imgui
float redColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float blueColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

// vertex
UINT veStartSlot;
UINT veNumBuffers;
ID3D11Buffer* veBuffer;
UINT Stride;
UINT veBufferOffset;
D3D11_BUFFER_DESC vedesc;

// index
ID3D11Buffer* inBuffer;
DXGI_FORMAT inFormat;
UINT inOffset; 
D3D11_BUFFER_DESC indesc;

// psgetConstantbuffers
UINT pscStartSlot;
UINT pscNumBuffers;
ID3D11Buffer* pscBuffer;
D3D11_BUFFER_DESC pscdesc;

struct propertiesModel currentParams;
std::unordered_set<propertiesModel> seenParams;
std::unordered_set<propertiesModel> wallhackParams;
int currentParamPosition = 1;
std::mutex g_propertiesModels;

// Z-Buffering variables
ID3D11DepthStencilState* m_DepthStencilState;
ID3D11DepthStencilState* m_DepthStencilStateFalse;
ID3D11DepthStencilState* m_origDepthStencilState;
UINT pStencilRef;

// Booleans for different wallhack models
bool bWallhack = false;
bool bCases = false;

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
        swap_chain->Release();
        device->Release();
        //context->Release();
        p_present_target = (present)p_vtable[8];
        return true;
    }
    return false;
}

// https://www.unknowncheats.me/forum/d3d-tutorials-and-source/75474-generateshader-directx11.html
HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b)
{
    const char* shaderTemplate = R"(
        struct VS_OUT
        {
            float4 Position : SV_Position;
            float4 Color : COLOR0;
        };

        float4 main(VS_OUT input) : SV_Target
        {
            float4 fake;
            fake.a = 1.0f;
            fake.r = %f;
            fake.g = %f;
            fake.b = %f;
            return fake;
        }
    )";

    char shaderCode[1024];
    sprintf_s(shaderCode, shaderTemplate, r, g, b);

    ID3DBlob* pBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pBlob, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        if (pBlob) pBlob->Release();
        return hr;
    }

    hr = pD3DDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);
    pBlob->Release();

    return hr;
}

void UpdateRedShader(float r, float g, float b, float a)
{
    if (pShaderRed) pShaderRed->Release();
    GenerateShader(p_device, &pShaderRed, r, g, b);
}
void UpdateBlueShader(float r, float g, float b, float a)
{
    if (pShaderBlue) pShaderBlue->Release();
    GenerateShader(p_device, &pShaderBlue, r, g, b);
}

void __stdcall hookD3D11DrawIndexedInstanced(ID3D11DeviceContext* p_context, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) 
{
    if (firstTime)
    {
        firstTime = false;
        D3D11_DEPTH_STENCIL_DESC depthStencilDescFalse;
        depthStencilDescFalse.DepthEnable = FALSE;
        depthStencilDescFalse.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDescFalse.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDescFalse.StencilEnable = FALSE;

        p_device->CreateDepthStencilState(&depthStencilDescFalse, &m_DepthStencilStateFalse);

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        p_device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);

        GenerateShader(p_device, &pShaderBlue, 0.0f, 0.0f, 1.0f);
    }
    
    // get stride & vedesc.ByteWidth
    p_context->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
    if (veBuffer)
        veBuffer->GetDesc(&vedesc);
    if (veBuffer != NULL) { veBuffer->Release(); veBuffer = NULL; }

    // get indesc.ByteWidth
    p_context->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
    if (inBuffer)
        inBuffer->GetDesc(&indesc);
    if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

    // get pscdesc.ByteWidth
    p_context->PSGetConstantBuffers(pscStartSlot, 1, &pscBuffer);
    if (pscBuffer != NULL)
        pscBuffer->GetDesc(&pscdesc);
    if (pscBuffer != NULL) { pscBuffer->Release(); pscBuffer = NULL; }

    propertiesModel paramsModelInstanced;
    paramsModelInstanced.stride = Stride;
    paramsModelInstanced.vedesc_ByteWidth = vedesc.ByteWidth;
    paramsModelInstanced.indesc_ByteWidth = indesc.ByteWidth;
    paramsModelInstanced.pscdesc_ByteWidth = pscdesc.ByteWidth;
    g_propertiesModels.lock();
    seenParams.insert(paramsModelInstanced);
    g_propertiesModels.unlock();

    if (!drawIndexedInstanced)
    {
        std::cout << "[+] DrawIndexedInstanced Hooked succesfully" << std::endl;
        drawIndexedInstanced = true;
        // Set this for the first time its called
        currentParams = paramsModelInstanced;
    }
    auto current = seenParams.find(currentParams);

    if ((wallhackParams.find(paramsModelInstanced) != wallhackParams.end()) && bShader)
    {
        p_context->OMGetDepthStencilState(&m_origDepthStencilState, &pStencilRef);

        auto applyWallhackAndChams = [&](ID3D11PixelShader* throughWallShader, ID3D11PixelShader* visibleShader)
        {
                // Deactivated depth stencil (visible through walls)
                p_context->OMSetDepthStencilState(m_DepthStencilStateFalse, pStencilRef);
                p_context->PSSetShader(throughWallShader, NULL, NULL);
                fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

                // Activated depth stencil (normal visibility)
                p_context->OMSetDepthStencilState(m_DepthStencilState, pStencilRef);
                p_context->PSSetShader(visibleShader, NULL, NULL);
                fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        };
        bool drawn = false;

        if (bWallhack && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 1308520)
        {
            // PlayerModel
            applyWallhackAndChams(pShaderRed, pShaderBlue);
            drawn = true;
        }
        else if (bCases && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 178560)
        {
            // Cases
            applyWallhackAndChams(pShaderRed, pShaderBlue);
            drawn = true;
        }
        if (drawn)
        {
            p_context->OMSetDepthStencilState(m_origDepthStencilState, pStencilRef);
            SAFE_RELEASE(m_origDepthStencilState);
            return;
        }
    }
    fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
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

            pDeviceContextVTable = (DWORD_PTR*)p_context;
            pDeviceContextVTable = (DWORD_PTR*)pDeviceContextVTable[0];
            fnID3D11DrawIndexedInstanced_target = (ID3D11DrawIndexedInstanced)pDeviceContextVTable[20];

            if (MH_CreateHook(reinterpret_cast<void**>(fnID3D11DrawIndexedInstanced_target), &hookD3D11DrawIndexedInstanced, reinterpret_cast<void**>(&fnID3D11DrawIndexedInstanced)) != MH_OK) {
                printf_s("Failed to create DrawIndexedInstanced hook\n");
            }
            else if (MH_EnableHook(fnID3D11DrawIndexedInstanced_target) != MH_OK) {
                printf_s("Failed to enable DrawIndexedInstanced hook\n");
            }
            else {
                printf_s("[+] DrawIndexedInstanced hook created and enabled successfully\n");
            }

            GenerateShader(p_device, &pShaderRed, 1.0f, 0.0f, 0.0f);
            
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
            // Checkbox to enable wallhack
            if (ImGui::Checkbox("Wallhack", &bWallhack))
            {
                bShader = bWallhack || bCases;
            }

            // Checkbox to enable cases
            if (ImGui::Checkbox("Cases", &bCases))
            {
                bShader = bWallhack || bCases;
            }
            
            // Color edit for wallhack and visible models
            if (ImGui::ColorEdit4("Wallhack Color", redColor))
            {
                UpdateRedShader(redColor[0], redColor[1], redColor[2], redColor[3]);
            }
            if (ImGui::ColorEdit4("Visible Color", blueColor))
            {
                UpdateBlueShader(blueColor[0], blueColor[1], blueColor[2], blueColor[3]);
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Aimbot"))
        {
            static bool enableAimbot = false;
            static float aimbotFOV = 0.5f;
            ImGui::Checkbox("Enable Aimbot", &enableAimbot);
            ImGui::SliderFloat("FOV", &aimbotFOV, 0.0f, 1.0f, "%.1f");

            /*if (ImGui::TreeNode("AI Aimbot"))
            {
                ImGui::TreePop();
            }*/
            ImGui::TreePop();
        }

        /*if (ImGui::TreeNode("Misc"))
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
        }*/
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

void setupWallhack() {
    propertiesModel wallhackParamsItem;
    // Case
    wallhackParamsItem.stride = 40;
    wallhackParamsItem.vedesc_ByteWidth = 178560;
    wallhackParamsItem.indesc_ByteWidth = 23856;
    wallhackParamsItem.pscdesc_ByteWidth = 2080;
    wallhackParams.insert(wallhackParamsItem);
    // PlayerModel
    wallhackParamsItem.stride = 40;
    wallhackParamsItem.vedesc_ByteWidth = 1308520;
    wallhackParamsItem.indesc_ByteWidth = 371628;
    wallhackParamsItem.pscdesc_ByteWidth = 2080;
    wallhackParams.insert(wallhackParamsItem);
    printf_s("[+] Wallhack params done\n");
}

bool AreHooksIntact()
{
    // Check if hooks are still valid
    return (MH_EnableHook(MH_ALL_HOOKS) == MH_OK);
}

// main code
int WINAPI gui::RunGUI()
{   
    printf_s("RUNGUI\n");
    if (!gui::get_present_pointer())
    {
        return 1;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        return 1;
    }

    setupWallhack();

    if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &gui::detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
        return 1;
    }
    if (MH_EnableHook(p_present_target) != MH_OK) {
        return 1;
    }

    //if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, reinterpret_cast<void**>(&pPeekMessageA), reinterpret_cast<void**>(&pPeekMessageATarget)) != MH_OK) {
    //    return 1;
    //}
    
    MH_STATUS testStatus = MH_CreateHook(isShootinggTarget, &detourIsShooting, reinterpret_cast<LPVOID*>(&isShootingg));
    if (testStatus != MH_OK)
    {
        std::string sStatus = MH_StatusToString(testStatus);
        std::cout << sStatus << std::endl;
        return 1;
    }

    while (true) {
        Sleep(50);
        /*bool test = AreHooksIntact();
        std::cout << "Hooks Intact: " << test << std::endl;*/
    }

    std::cin.get();

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

// Forward declare message handler from imgui_impl_win32.cpp
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
