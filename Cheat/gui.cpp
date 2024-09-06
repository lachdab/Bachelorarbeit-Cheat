#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// SOURCE: https://github.com/CasualCoder91/DX11Hook/blob/master/Main.cpp
// Used as a Codebase

// MINHOOK
// SOURCE: https://github.com/TsudaKageyu/minhook
#include "ext/MinHook/MinHook.h"
#pragma comment(lib, "ext/MinHook/libMinHook.x64.lib")

// IMGUI
// SOURCE: https://github.com/ocornut/imgui
#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_win32.h"
#include "ext/imgui/imgui_impl_dx11.h"

// Cheat Headerfiles
#include "aimbot.h"
#include "wallhack.h"

// OTHER
#include <d3d11.h>
#include <string>
#include "gui.h"
#include <iostream>
#include <d3dcompiler.h>
// SOURCE: https://www.unknowncheats.me/forum/unity/476697-il2cpp-resolver.html
#include <IL2CPP_Resolver/IL2CPP_Resolver.hpp>
#include "helper.h"
#include "functions.h"
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

WNDPROC oWndProc;
HINSTANCE gui::dll_handle = nullptr;
HWND window = NULL;

ID3D11RenderTargetView* mainRenderTargetView = NULL;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present pPresent;
present pPresentTarget;

// Hooking DrawIndexedInstanced from DX11 for Playermodel and Cases
typedef void(__stdcall* ID3D11DrawIndexedInstanced)(ID3D11DeviceContext* p_context, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
ID3D11DrawIndexedInstanced fnID3D11DrawIndexedInstanced = nullptr;
DWORD_PTR* pDeviceContextVTable = NULL;

// SOURCE: https://github.com/CasualCoder91/DX11Hook/blob/master/Main.cpp
// Modified: added processName and Functions::GetHandlerByWindowTitle()
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
    // Creation of the dx11 device and swap chain
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

// SOURCE: https://www.unknowncheats.me/forum/d3d-tutorials-and-source/75474-generateshader-directx11.html
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

    hr = Vars::pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);
    pBlob->Release();

    return hr;
}

// SOURCE: https://niemand.com.ar/2019/01/08/fingerprinting-models-when-hooking-directx-vermintide-2/
// SOURCE: https://niemand.com.ar/2019/01/13/creating-your-own-wallhack/
// Implemented this function based on these 2 sources
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

        Vars::pDevice->CreateDepthStencilState(&depthStencilDescFalse, &m_DepthStencilStateFalse);

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        Vars::pDevice->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);

        gui::GenerateShader(&pShaderBlue, 0.0f, 0.0f, 1.0f);
    }
    
    // Get stride & vedesc.ByteWidth
    p_context->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
    if (veBuffer)
        veBuffer->GetDesc(&vedesc);
    if (veBuffer != NULL) { veBuffer->Release(); veBuffer = NULL; }

    // Get indesc.ByteWidth
    p_context->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
    if (inBuffer)
        inBuffer->GetDesc(&indesc);
    if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

    // Get pscdesc.ByteWidth
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
        // Save the original depth stencil state
        p_context->OMGetDepthStencilState(&m_origDepthStencilState, &pStencilRef);

        // Lambda function to apply wallhack and chams effects
        auto applyWallhackAndChams = [&](ID3D11PixelShader* throughWallShader, ID3D11PixelShader* visibleShader)
        {
                // Deactivate depth stencil (make objects visible through walls)
                p_context->OMSetDepthStencilState(m_DepthStencilStateFalse, pStencilRef);
                p_context->PSSetShader(throughWallShader, NULL, NULL);
                fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

                // Activate depth stencil (normal visibility)
                p_context->OMSetDepthStencilState(m_DepthStencilState, pStencilRef);
                p_context->PSSetShader(visibleShader, NULL, NULL);
                fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        };
        bool drawn = false;

        // Check if the parameters match for player models
        if (Vars::wallhack && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 1308520)
        {
            // Apply wallhack and chams for player models
            applyWallhackAndChams(pShaderRed, pShaderBlue);
            drawn = true;
        }
        // Check if the parameters match for cases
        else if (Vars::cases && paramsModelInstanced.stride == 40 && paramsModelInstanced.vedesc_ByteWidth == 178560)
        {
            // Apply wallhack and chams for cases
            applyWallhackAndChams(pShaderRed, pShaderBlue);
            drawn = true;
        }
        if (drawn)
        {
            // Restore the original depth stencil state
            p_context->OMSetDepthStencilState(m_origDepthStencilState, pStencilRef);
            SAFE_RELEASE(m_origDepthStencilState);
            return;
        }
    }
    fnID3D11DrawIndexedInstanced(p_context, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

// SOURCE: https://github.com/CasualCoder91/DX11Hook/blob/master/Main.cpp
// Modified: for my own needs
static long __stdcall gui::hkPresent(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) 
{   
    void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());

    if (!Vars::init) {
        if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&Vars::pDevice)))
        {
            Vars::pDevice->GetImmediateContext(&Vars::pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            p_swap_chain->GetDesc(&sd);
            ImGui::CreateContext();
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            Vars::pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)gui::WndProc);

            pDeviceContextVTable = (DWORD_PTR*)Vars::pContext;
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
            ImGui_ImplDX11_Init(Vars::pDevice, Vars::pContext);
            Vars::pSwapChain = p_swap_chain;
            Vars::init = true;
        }
        else
            return pPresent(p_swap_chain, sync_interval, flags);
    }

    Vars::pContext->RSGetViewports(&Vars::vps, &Vars::viewport);
    Vars::screenSize = { Vars::viewport.Width, Vars::viewport.Height };
    Vars::screenCenter = { Vars::viewport.Width / 2.0f, Vars::viewport.Height / 2.0f };

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();

    // Own contribution
    if (Vars::showImGuiMenu) {
        if (ImGui::Begin("Cheat Menu", nullptr, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("with F1 show/hide cheat menu");
            ImGui::SetWindowPos(ImVec2(232, 140), ImGuiCond_Once);
            ImGui::SetWindowSize(ImVec2(591, 546), ImGuiCond_Once);
            if (ImGui::Button("ESP"))
            {
                Vars::tab = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Aimbot"))
            {
                Vars::tab = 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("AI Aimbot"))
            {
                Vars::tab = 2;
            }
            ImGui::SameLine();
            if (ImGui::Button("Misc"))
            {
                Vars::tab = 3;
            }
            static char modelPathBuffer[256];
            static bool modelPathInitialized = false;
            std::string previewValue;
            if (!modelPathInitialized) {
                strncpy_s(modelPathBuffer, AI::modelPath.c_str(), sizeof(modelPathBuffer) - 1);
                modelPathBuffer[sizeof(modelPathBuffer) - 1] = '\0';
                modelPathInitialized = true;
            }

            switch (Vars::tab)
            {
            case 0:
                // Checkbox to enable Players
                if (ImGui::Checkbox("Enable Players", &Vars::wallhack))
                {
                    Vars::shader = Vars::wallhack || Vars::cases;
                }
                // Checkbox to enable cases
                if (ImGui::Checkbox("Enable Cases", &Vars::cases))
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
                break;
            case 1:
                ImGui::Checkbox("Enable", &Vars::aimbot);

                ImGui::Checkbox("Aim FOV", &Vars::fovCheck);
                if (Vars::fovCheck)
                {
                    ImGui::SliderFloat("Aimbot FOV", &Vars::aimbotFov, 0, 300.0f);
                }

                ImGui::SliderFloat("Aimbot Smoothing", &Vars::smooth, 1, 10.0f);

                ImGui::Combo("Aim Bone", &Vars::boneSelected, Vars::bones, IM_ARRAYSIZE(Vars::bones));
                if (ImGui::Combo("Aim Key", &Vars::selectedKeyIndex, Vars::keyNames, IM_ARRAYSIZE(Vars::keyNames)))
                {
                    Vars::aimkey = Vars::keyValues[Vars::selectedKeyIndex];
                }
                break;
            case 2:
                ImGui::Checkbox("Enable", &AI::enableAIAimbot);
                ImGui::Checkbox("Auto Aim", &AI::autoAim);
                ImGui::SliderFloat("Aim Speed", &ExtendedAI::aimSpeed, 0.01f, 1.0f, "%.2f");
                ImGui::SliderFloat("Human Error Chance", &ExtendedAI::humanErrorChance, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Perfect Aim Chance", &ExtendedAI::perfectAimChance, 0.0f, 0.1f, "%.3f");
                ImGui::SliderFloat("Max Human Error (pixels)", &ExtendedAI::maxHumanError, 0.0f, 20.0f, "%.1f");
                for (int i = 0; i < IM_ARRAYSIZE(AI::classNames); i++) {
                    if (AI::classSelected[i]) {
                        if (!previewValue.empty()) previewValue += ", ";
                        previewValue += AI::classNames[i];
                    }
                }
                if (previewValue.empty()) previewValue = "Select Classes";
                if (ImGui::BeginCombo("Classes to Show", previewValue.c_str())) {
                    for (int i = 0; i < IM_ARRAYSIZE(AI::classNames); i++) {
                        bool isSelected = AI::classSelected[i];
                        if (ImGui::Selectable(AI::classNames[i], &isSelected)) {
                            AI::classSelected[i] = isSelected;
                        }
                    }
                    ImGui::EndCombo();
                }
                AI::classesToShow.clear();
                for (int i = 0; i < IM_ARRAYSIZE(AI::classNames); i++) {
                    if (AI::classSelected[i]) {
                        AI::classesToShow.push_back(AI::classNames[i]);
                    }
                }

                ImGui::Checkbox("Enable Cuda", &AI::enableCuda);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("If you have an Nvidia graphics card, you can ignore this checkbox. If not, uncheck this box.");

                if (ImGui::InputText("Model Path", modelPathBuffer, sizeof(modelPathBuffer))) {
                    AI::modelPath = modelPathBuffer;
                    AI::pathChanged = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Path to the AI model. Changes require a restart of the aimbot.");

                if (ImGui::CollapsingHeader("for more FPS"))
                {
                    ImGui::Checkbox("Enable Frame Limit", &AI::enableFrameLimit);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Limits the frequency of AI inference to save resources. May affect response time and accuracy.");

                    ImGui::SliderInt("Inference Interval", &AI::inferenceInterval, 1, 60);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Number of frames between AI inferences. Higher values save resources but may increase response time and affect accuracy.");
                        ImGui::SetTooltip("Inference Interval:\n"
                            "1 = Every frame (fastest response, highest CPU usage)\n"
                            "30 = Every half second at 60 FPS\n"
                            "60 = Once per second at 60 FPS\n"
                            "Recommended: 5-15 for good balance");
                    }
                }
                break;
            case 3:
                ImGui::Checkbox("Enable Fov Changer", &Vars::fovChanger);
                if (Vars::fovChanger)
                {
                    ImGui::SliderFloat("##CamFOV", &Vars::cameraFOV, 20, 180, "Camera FOV: %.0f");
                }
                break;
            }
        }
        ImGui::End();
    }
    Functions::GuiLogic();

    ImGui::EndFrame();
    ImGui::Render();

    Vars::pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
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

// main code
int WINAPI gui::RunGUI()
{
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
    Functions::FindOffsets();

    if (!gui::GetPresentPointer())
    {
        return 1;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        return 1;
    }

    Functions::SetupWallhack();
    Functions::InitializeAISettings();

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

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Functions::PlayerCache, NULL, NULL, NULL);

    while (true) {
        Sleep(50);
    }

    // Cleanup
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
    if (Vars::pContext) { Vars::pContext->Release(); Vars::pContext = NULL; }
    if (Vars::pDevice) { Vars::pDevice->Release(); Vars::pDevice = NULL; }
    if (Vars::pSwapChain) { Vars::pSwapChain->Release(); Vars::pSwapChain = NULL; }
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
    