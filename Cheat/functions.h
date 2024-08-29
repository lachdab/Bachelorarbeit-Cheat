#pragma once
#include "IL2CPP_Resolver/IL2CPP_Resolver.hpp"
#include "helper.h"
#include <imgui/imgui.h>

namespace Functions
{
#pragma region Hooks
    void(__fastcall* OrigSetFieldOfView)(Unity::CCamera*, float);
    void hkSetFieldOfView(Unity::CCamera* camera, float fov)
    {
        if (Vars::fovChanger)
        {
            fov = Vars::cameraFOV;
        }
        return OrigSetFieldOfView(camera, fov);
    }
#pragma endregion
#pragma region Helper / Normal Functions
    void ImGuiCustomStyle() 
    {
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
    HWND GetHandlerByWindowTitle(const std::wstring& windowTitle) 
    {
        HWND window = FindWindowW(NULL, windowTitle.c_str());
        if (window != NULL) 
        {
            DWORD processId;
            DWORD threadId = GetWindowThreadProcessId(window, &processId);
            if (threadId != 0) 
            {
                return window;
            }
            else 
            {
                return window;
            }
        }
        else 
        {
            return window;
        }
        return window;
    }
    bool WorldToScreen(Unity::Vector3 world, Unity::Vector2& screen)
    {
        Unity::CCamera* CameraMain = Unity::Camera::GetMain();
        if (!CameraMain) 
        {
            return false;
        }

        Unity::Vector3 buffer = CameraMain->CallMethodSafe<Unity::Vector3>("WorldToScreenPoint", world, 2);

        // Check if point is on screen
        if (buffer.x > Vars::screenSize.x || buffer.y > Vars::screenSize.y || buffer.x < 0 || buffer.y < 0 || buffer.z < 0)
        {
            return false;
        }

        // Check if point is in view
        if (buffer.z > 0.0f)
        {
            screen = Unity::Vector2(buffer.x, Vars::screenSize.y - buffer.y);
        }

        // Check if point is in view
        if (screen.x > 0 || screen.y > 0)
        {
            return true;
        }
        return false;
    }
    void MouseMove(float tarx, float tary, float X, float Y, int smooth)
    {
        float ScreenCenterX = (X / 2);
        float ScreenCenterY = (Y / 2);
        float TargetX = 0;
        float TargetY = 0;

        smooth = smooth + 3;

        if (tarx != 0)
        {
            if (tarx > ScreenCenterX)
            {
                TargetX = -(ScreenCenterX - tarx);
                TargetX /= smooth;
                if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
            }

            if (tarx < ScreenCenterX)
            {
                TargetX = tarx - ScreenCenterX;
                TargetX /= smooth;
                if (TargetX + ScreenCenterX < 0) TargetX = 0;
            }
        }

        if (tary != 0)
        {
            if (tary > ScreenCenterY)
            {
                TargetY = -(ScreenCenterY - tary);
                TargetY /= smooth;
                if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
            }

            if (tary < ScreenCenterY)
            {
                TargetY = tary - ScreenCenterY;
                TargetY /= smooth;
                if (TargetY + ScreenCenterY < 0) TargetY = 0;
            }
        }
        mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);
    }
    float GetDistance(Unity::Vector3 a, Unity::Vector3 b)
    {
        return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    }
    Unity::CGameObject* GetNearestPlayer(std::vector<Unity::CGameObject*> list, Unity::CGameObject* localplayer)
    {
        Unity::CGameObject* nearestPlayer = nullptr;
        float nearestDistance = FLT_MAX;

        for (int i = 0; i < list.size(); i++)
        {
            Unity::CGameObject* currentPlayer = list[i];

            if (!currentPlayer)
                continue;

            float distance = GetDistance(localplayer->GetTransform()->GetPosition(), currentPlayer->GetTransform()->GetPosition());
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestPlayer = currentPlayer;
            }
        }
        return nearestPlayer;
    }
    bool ExecAimbot(Unity::CGameObject* target, Unity::Vector2 bodyTarget)
    {
        if (Vars::fovCheck)
        {
            if (bodyTarget.x > (Vars::screenCenter.x + Vars::aimbotFov))
                return false;
            if (bodyTarget.x < (Vars::screenCenter.x - Vars::aimbotFov))
                return false;
            if (bodyTarget.y > (Vars::screenCenter.y + Vars::aimbotFov))
                return false;
            if (bodyTarget.y < (Vars::screenCenter.y - Vars::aimbotFov))
                return false;
        }

        if (GetAsyncKeyState(Vars::aimkey)) 
        {
            if (Vars::targetPlayer == nullptr) 
            {
                Vars::targetPlayer = target;
            }

            if (Vars::targetPlayer == target) 
            {
                MouseMove(bodyTarget.x, bodyTarget.y, Vars::screenSize.x, Vars::screenSize.y, Vars::smooth);
            }
        }
        else 
        {
            Vars::targetPlayer = nullptr;
        }
        return true;
    }
    // NOTE: WICHTIG, da sich bei jedem start des spiels irgendwie das offset der kamera ändert. Mit dieser funktion können wir dynamisch das offset holen
    bool FindOffsets() 
    {
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
    cv::Mat CaptureFrameGPU(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain) 
    {
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr)) 
        {
            std::cerr << "Fehler beim Abrufen des Backbuffers." << std::endl;
            return cv::Mat();
        }

        // Erstellen einer Staging-Textur für den CPU-Zugriff
        D3D11_TEXTURE2D_DESC desc;
        backBuffer->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        ID3D11Texture2D* stagingTexture = nullptr;
        hr = device->CreateTexture2D(&desc, nullptr, &stagingTexture);
        if (FAILED(hr)) 
        {
            std::cerr << "Fehler beim Erstellen der Staging-Textur." << std::endl;
            backBuffer->Release();
            return cv::Mat();
        }

        // Kopieren des Backbuffers in die Staging-Textur
        context->CopyResource(stagingTexture, backBuffer);
        backBuffer->Release();

        // Mappen der Staging-Textur, um auf die Bilddaten zuzugreifen
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) 
        {
            std::cerr << "Fehler beim Mappen der Staging-Textur." << std::endl;
            stagingTexture->Release();
            return cv::Mat();
        }

        // Erstellen eines OpenCV-Mat-Objekts aus den Bilddaten
        cv::Mat image(desc.Height, desc.Width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);
        cv::Mat result;
        cv::cvtColor(image, result, cv::COLOR_BGRA2BGR);

        // Entmappen der Staging-Textur
        context->Unmap(stagingTexture, 0);
        stagingTexture->Release();

        return result;
    }
    void InitializeAISettings() 
    {
        for (int i = 0; i < IM_ARRAYSIZE(AI::classNames); i++) 
        {
            AI::classSelected[i] = (std::find(AI::classesToShow.begin(), AI::classesToShow.end(), AI::classNames[i]) != AI::classesToShow.end());
        }
    }
    void SetupWallhack() 
    {
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
    void GuiLogic()
    {
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
                if (Vars::boneSelected == 0) 
                {
                    transform = playerTransform->FindChild(Vars::marineHeadPath);
                    if (transform) 
                    {
                        targetPos = transform->GetPosition();
                        if (Functions::WorldToScreen(targetPos, inView)) 
                        {
                            Functions::ExecAimbot(currentPlayer, inView);
                        }
                    }

                    transform = playerTransform->FindChild(Vars::soldierHeadPath);
                    if (transform) 
                    {
                        targetPos = transform->GetPosition();
                        if (Functions::WorldToScreen(targetPos, inView)) 
                        {
                            Functions::ExecAimbot(currentPlayer, inView);
                        }
                    }
                }
                else if (Vars::boneSelected == 1) 
                {
                    transform = playerTransform->FindChild(Vars::marineChestPath);
                    if (transform) 
                    {
                        targetPos = transform->GetPosition();
                        if (Functions::WorldToScreen(targetPos, inView)) 
                        {
                            Functions::ExecAimbot(currentPlayer, inView);
                        }
                    }

                    transform = playerTransform->FindChild(Vars::soldierChestPath);
                    if (transform) 
                    {
                        targetPos = transform->GetPosition();
                        if (Functions::WorldToScreen(targetPos, inView)) 
                        {
                            Functions::ExecAimbot(currentPlayer, inView);
                        }
                    }
                }
            }
        }
        if (Vars::fovCheck) 
        {
            ImGui::GetForegroundDrawList()->AddCircle(ImVec2(Vars::screenCenter.x, Vars::screenCenter.y), Vars::aimbotFov, ImColor(255, 255, 255), 360);
        }
        if (AI::enableAIAimbot)
        {
            if (!AI::initAI)
            {
                try 
                {
                    AI::inferenceModel = new YOLO::Inference(AI::modelPath, cv::Size(640, 640), AI::enableCuda);
                    AI::initAI = true;
                    std::cout << "AI model initialized successfully." << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error initializing AI model: " << e.what() << std::endl;
                    AI::initAI = false;
                    if (AI::inferenceModel) 
                    {
                        AI::inferenceModel = nullptr;
                    }
                }
            }
            cv::Mat frame = Functions::CaptureFrameGPU(Vars::pDevice, Vars::pContext, Vars::g_pSwapChain);
            if (!frame.empty())
            {
                std::vector<YOLO::Detection> output;
                if (AI::enableFrameLimit)
                {
                    AI::frameCounter++;
                    if (AI::frameCounter >= AI::inferenceInterval)
                    {
                        cv::Mat resizedFrame;
                        cv::resize(frame, resizedFrame, cv::Size(640, 640));
                        output = AI::inferenceModel->runInference(resizedFrame);
                        AI::frameCounter = 0;
                    }
                }
                else if (!AI::enableFrameLimit)
                {
                    output = AI::inferenceModel->runInference(frame);
                }
                YOLO::DrawBoundingBoxes(output, AI::classesToShow);

                if (!output.empty())
                {
                    ExtendedAI::screenSizeX = Vars::screenSize.x;
                    ExtendedAI::screenSizeY = Vars::screenSize.y;

                    auto prioTargets = ExtendedAI::PrioritizeBodyParts(output, AI::classesToShow);
                    if (!prioTargets.empty())
                    {
                        const auto& target = prioTargets[0];
                        cv::Point2f currentTargetPos(
                            target.box.x + target.box.width / 2.0f,
                            target.box.y + target.box.height / 2.0f
                        );

                        static cv::Point2f currentAimPoint(Vars::screenSize.x / 2, Vars::screenSize.y / 2);
                        cv::Point2f newAimPoint = ExtendedAI::SimulateHumanAiming(currentAimPoint, currentTargetPos, ExtendedAI::aimSpeed);

                        if (AI::autoAim || (GetAsyncKeyState(VK_MENU) & 0x8000))
                        {
                            Functions::MouseMove(newAimPoint.x, newAimPoint.y, Vars::screenSize.x, Vars::screenSize.y, 1);
                            currentAimPoint = newAimPoint;
                        }
                    }
                }
            }
            if (AI::pathChanged) 
            {
                AI::initAI = false;
                delete AI::inferenceModel;
                AI::inferenceModel = nullptr;
                AI::pathChanged = false;
            }
        }
    }
#pragma endregion
#pragma region AI
#pragma endregion
}
