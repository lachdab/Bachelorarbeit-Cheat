#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MINHOOK
#include "ext/MinHook/MinHook.h"
#pragma comment(lib, "ext/MinHook/libMinHook.x64.lib")

// IMGUI
#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_win32.h"
#include "ext/imgui/imgui_impl_dx11.h"

//#include "aimbot.h"
#include "wallhack.h"

// OTHER
#include <d3d11.h>
#include <string>
#include "gui.h"
#include <iostream>
#include <d3dcompiler.h>
#include <IL2CPP_Resolver/IL2CPP_Resolver.hpp>
#include "helper.h"
#include "functions.h"
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

WNDPROC oWndProc;
HINSTANCE gui::dll_handle = nullptr;
HWND window = NULL;

ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present pPresent;
present pPresentTarget;

// Hooking DrawIndexedInstanced from DX11 for PlayerModel and Cases
typedef void(__stdcall* ID3D11DrawIndexedInstanced)(ID3D11DeviceContext* p_context, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
ID3D11DrawIndexedInstanced fnID3D11DrawIndexedInstanced = nullptr;
DWORD_PTR* pDeviceContextVTable = NULL;

bool gui::GetPresentPointer()
{
    std::wstring processName = L"Bachelorarbeit2.0";

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = Functions::GetHandlerByWindowTitle(processName);
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
        pPresentTarget = (present)p_vtable[8];
        return true;
    }
    return false;
}

// https://www.unknowncheats.me/forum/d3d-tutorials-and-source/75474-generateshader-directx11.html
HRESULT gui::GenerateShader(ID3D11PixelShader** pShader, float r, float g, float b)
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

    hr = pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);
    pBlob->Release();

    return hr;
}

void __stdcall hkD3D11DrawIndexedInstanced(ID3D11DeviceContext* p_context, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) 
{
    if (Vars::firstTime)
    {
        Vars::firstTime = false;
        D3D11_DEPTH_STENCIL_DESC depthStencilDescFalse;
        depthStencilDescFalse.DepthEnable = FALSE;
        depthStencilDescFalse.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDescFalse.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDescFalse.StencilEnable = FALSE;

        pDevice->CreateDepthStencilState(&depthStencilDescFalse, &m_DepthStencilStateFalse);

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        pDevice->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);

        gui::GenerateShader(&pShaderBlue, 0.0f, 0.0f, 1.0f);
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

    if (!Vars::drawIndexedInstanced)
    {
        Vars::drawIndexedInstanced = true;
        // Set this for the first time its called
        currentParams = paramsModelInstanced;
    }
    auto current = seenParams.find(currentParams);

    if ((wallhackParams.find(paramsModelInstanced) != wallhackParams.end()) && Vars::shader)
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

        if (Vars::wallhack && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 1308520)
        {
            // PlayerModel
            applyWallhackAndChams(pShaderRed, pShaderBlue);
            drawn = true;
        }
        else if (Vars::cases && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 178560)
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

static long __stdcall gui::hkPresent(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
    
    void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());

    if (!Vars::init) {
        if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            p_swap_chain->GetDesc(&sd);
            ImGui::CreateContext();
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)gui::WndProc);

            pDeviceContextVTable = (DWORD_PTR*)pContext;
            pDeviceContextVTable = (DWORD_PTR*)pDeviceContextVTable[0];

            if (MH_CreateHook(reinterpret_cast<void**>((ID3D11DrawIndexedInstanced)pDeviceContextVTable[20]), &hkD3D11DrawIndexedInstanced, reinterpret_cast<void**>(&fnID3D11DrawIndexedInstanced)) != MH_OK) {
                printf_s("Failed to create DrawIndexedInstanced hook\n");
            }
            else if (MH_EnableHook((ID3D11DrawIndexedInstanced)pDeviceContextVTable[20]) != MH_OK) {
                printf_s("Failed to enable DrawIndexedInstanced hook\n");
            }
            else {
                printf_s("[+] DrawIndexedInstanced hook created and enabled successfully\n");
            }

            gui::GenerateShader(&pShaderRed, 1.0f, 0.0f, 0.0f);
            
            Functions::ImGuiCustomStyle();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(pDevice, pContext);
            Vars::init = true;
        }
        else
            return pPresent(p_swap_chain, sync_interval, flags);
    }

    pContext->RSGetViewports(&Vars::vps, &Vars::viewport);
    Vars::screenSize = { Vars::viewport.Width, Vars::viewport.Height };
    Vars::screenCenter = { Vars::viewport.Width / 2.0f, Vars::viewport.Height / 2.0f };

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();

    if (Vars::showImGuiMenu) {
        ImGui::Begin("Cheat Menu");
        ImGui::Text("with F1 show/hide cheat menu");

        if (ImGui::TreeNode("ESP"))
        {
            // Checkbox to enable Players
            if (ImGui::Checkbox("Players", &Vars::wallhack))
            {
                Vars::shader = Vars::wallhack || Vars::cases;
            }

            // Checkbox to enable cases
            if (ImGui::Checkbox("Cases", &Vars::cases))
            {
                Vars::shader = Vars::wallhack || Vars::cases;
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
            ImGui::Checkbox("Aim", &Vars::aimbot);

            ImGui::Checkbox("Aim FOV", &Vars::fovCheck);
            if (Vars::fovCheck)
            {
                ImGui::SliderFloat("Aimbot FOV", &Vars::aimbotFov, 0, 300.0f);
            }

            ImGui::SliderFloat("Aimbot Smoothing", &Vars::smooth, 1, 10.0f);

            ImGui::Combo("Aim Bone", &Vars::boneSelected, Vars::bones, IM_ARRAYSIZE(Vars::bones));
            if (ImGui::Combo("Aim Key", &Vars::selectedKeyIndex, Vars::keyNames, IM_ARRAYSIZE(Vars::keyNames))) {
                Vars::aimkey = Vars::keyValues[Vars::selectedKeyIndex];
            }

            /*if (ImGui::TreeNode("AI Aimbot"))
            {
                ImGui::TreePop();
            }*/
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Misc"))
        {
            ImGui::Checkbox("Fov Changer", &Vars::fovChanger);
            if (Vars::fovChanger)
            {
                ImGui::SliderFloat("##CamFOV", &Vars::cameraFOV, 20, 180, "Camera FOV: %.0f");
            }
            ImGui::TreePop();
        }
        ImGui::End();
    }
    // NOTE: hier kommt der code der ausgeführt wird wenn variablen sich im menü ändern
    // NOTE: dies evtl. in eine eigene Funktion packen
    if (Vars::fovChanger)
    {
        Unity::CCamera* CameraMain = Unity::Camera::GetMain();
        if (CameraMain != nullptr)
        {
            CameraMain->CallMethodSafe<void*>("set_fieldOfView", Vars::cameraFOV);
        }
    }
    if (Vars::aimbot)
    {
        for (int i = 0; i < Vars::playerList.size(); i++)
        {
            Unity::CGameObject* currentPlayer = Vars::playerList[i];
            if (!currentPlayer) continue;

            Unity::CTransform* playerTransform = currentPlayer->GetTransform();
            if (!playerTransform) continue;

            Unity::Vector3 targetPos;
            Unity::CTransform* transform = nullptr;
            Unity::Vector2 inView;
            if (Vars::boneSelected == 0) {
                transform = playerTransform->FindChild(Vars::marineHeadPath);
                if (transform) {
                    targetPos = transform->GetPosition();
                    if (Functions::WorldToScreen(targetPos, inView)) {
                        Functions::ExecAimbot(currentPlayer, inView);
                    }
                }

                transform = playerTransform->FindChild(Vars::soldierHeadPath);
                if (transform) {
                    targetPos = transform->GetPosition();
                    if (Functions::WorldToScreen(targetPos, inView)) {
                        Functions::ExecAimbot(currentPlayer, inView);
                    }
                }
            }
            else if (Vars::boneSelected == 1) {
                transform = playerTransform->FindChild(Vars::marineChestPath);
                if (transform) {
                    targetPos = transform->GetPosition();
                    if (Functions::WorldToScreen(targetPos, inView)) {
                        Functions::ExecAimbot(currentPlayer, inView);
                    }
                }

                transform = playerTransform->FindChild(Vars::soldierChestPath);
                if (transform) {
                    targetPos = transform->GetPosition();
                    if (Functions::WorldToScreen(targetPos, inView)) {
                        Functions::ExecAimbot(currentPlayer, inView);
                    }
                }
            }
        }
    }
    if (Vars::fovCheck) {
        ImGui::GetForegroundDrawList()->AddCircle(ImVec2(Vars::screenCenter.x, Vars::screenCenter.y), Vars::aimbotFov, ImColor(255, 255, 255), 360);
    }

    ImGui::EndFrame();
    ImGui::Render();

    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    IL2CPP::Thread::Detach(m_pThisThread);

    return pPresent(p_swap_chain, sync_interval, flags);
}

DWORD __stdcall gui::EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeConsole();
    FreeLibraryAndExitThread(gui::dll_handle, 0);
    Sleep(100);
    return 0;
}

void SetupWallhack() {
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

// NOTE: WICHTIG, da sich bei jedem start des spiels irgendwie das offset der kamera ändert. Mit dieser funktion können wir dynamisch das offset holen
bool FindOffsets() {
    Unity::il2cppClass* CameraClass = IL2CPP::Class::Find("UnityEngine.Camera");
    Offsets::setFieldOfView = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(CameraClass, "set_fieldOfView");
    return true;
}

bool PlayerCache()
{
    while (true)
    {
        void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
        Vars::localPlayer = NULL;
        Vars::playerList.clear();
        auto list = Unity::Object::FindObjectsOfType<Unity::CComponent>("UnityEngine.Rigidbody");
        for (int i = 0; i < list->m_uMaxLength; i++)
        {
            if (!list->operator[](i))
                continue;
            auto obj = list->operator[](i);
            if (!obj)
                continue;

            std::string objectname = obj->GetName()->ToString();
            if (objectname.find("[Player:") == 0)
            {
                Vars::playerList.push_back(list->operator[](i)->GetGameObject());
            }
        }
        IL2CPP::Thread::Detach(m_pThisThread);
        Sleep(1000);
    }
}

// main code
int WINAPI gui::RunGUI()
{   
    // TODO: wenn später die konsole entfernt wird, dann diese if überarbeiten
    if (IL2CPP::Initialize(true))
    {
        printf("[DEBUG] Il2cpp API initialized\n");
    }
    else
    {
        printf("[DEBUG] Il2cpp API initialized failed, quitting\n");
        Sleep(500);
        exit(0);
    }
    Vars::Base = (uintptr_t)GetModuleHandleA(NULL);
    Vars::GameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
    Vars::UnityPlayer = (uintptr_t)GetModuleHandleA("UnityPlayer.dll");
    IL2CPP::Callback::Initialize();
    FindOffsets();

    if (!gui::GetPresentPointer())
    {
        return 1;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        return 1;
    }

    SetupWallhack();

    if (MH_CreateHook(reinterpret_cast<void**>(pPresentTarget), &gui::hkPresent, reinterpret_cast<void**>(&pPresent)) != MH_OK) {
        return 1;
    }
    if (MH_EnableHook(pPresentTarget) != MH_OK) {
        return 1;
    }

    if (MH_CreateHook(reinterpret_cast<void**>(Offsets::setFieldOfView), &Functions::hkSetFieldOfView, reinterpret_cast<void**>(&Functions::OrigSetFieldOfView)) != MH_OK) {
        return 1;
    }
    if (MH_EnableHook(reinterpret_cast<void**>(Offsets::setFieldOfView)) != MH_OK) {
        return 1;
    }

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)PlayerCache, NULL, NULL, NULL);

    while (true) {
        Sleep(50);
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
    if (pContext) { pContext->Release(); pContext = NULL; }
    if (pDevice) { pDevice->Release(); pDevice = NULL; }
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
            Vars::showImGuiMenu = !Vars::showImGuiMenu;
        }
    }

    if (Vars::showImGuiMenu)
    {
        ClipCursor(NULL);
        io.MouseDrawCursor = true;
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }
    else {
        io.MouseDrawCursor = false;
        RECT rect;
        GetClientRect(hWnd, &rect);
        ClipCursor(&rect);
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}
    