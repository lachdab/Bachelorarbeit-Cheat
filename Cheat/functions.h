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
    // Own contribution
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
    // Own contribution
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
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
    // Modified: CallMethodSafe
    bool WorldToScreen(Unity::Vector3 world, Unity::Vector2& screen)
    {
        // Get the main camera from the game
        Unity::CCamera* CameraMain = Unity::Camera::GetMain();
        if (!CameraMain) 
        {
            return false; // Return false if the camera is not found
        }

        // Convert world coordinates to screen coordinates
        Unity::Vector3 buffer = CameraMain->CallMethodSafe<Unity::Vector3>("WorldToScreenPoint", world, 2);

        // Check if the converted screen coordinates are outside the screen bounds
        if (buffer.x > Vars::screenSize.x || buffer.y > Vars::screenSize.y || buffer.x < 0 || buffer.y < 0 || buffer.z < 0) 
        {
            return false;
        }

        // If the point is in front of the camera, adjust the y-coordinate and set the screen position
        if (buffer.z > 0.0f) 
        {
            screen = Unity::Vector2(buffer.x, Vars::screenSize.y - buffer.y);
        }

        // Return true if the screen position is valid
        if (screen.x > 0 || screen.y > 0) 
        {
            return true;
        }
        return false; // Return false if the screen position is not valid
    }
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
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
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
    float GetDistance(Unity::Vector3 a, Unity::Vector3 b)
    {
        return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    }
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
    Unity::CGameObject* GetNearestPlayer(std::vector<Unity::CGameObject*> list, Unity::CGameObject* localplayer)
    {
        Unity::CGameObject* nearestPlayer = nullptr; // Initialize the nearest player as null
        float nearestDistance = FLT_MAX; // Set the initial nearest distance to the maximum possible float value

        // Iterate through the list of players
        for (int i = 0; i < list.size(); i++) 
        {
            Unity::CGameObject* currentPlayer = list[i];
            if (!currentPlayer) continue; // Skip if the current player is null

            // Calculate the distance between the local player and the current player
            float distance = GetDistance(localplayer->GetTransform()->GetPosition(), currentPlayer->GetTransform()->GetPosition());

            // Update the nearest player if the current player is closer
            if (distance < nearestDistance) 
            {
                nearestDistance = distance;
                nearestPlayer = currentPlayer;
            }
        }
        return nearestPlayer; // Return the nearest player found
    }
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
    bool ExecAimbot(Unity::CGameObject* target, Unity::Vector2 bodyTarget)
    {
        // Check if the target is within the field of view (FOV)
        if (Vars::fovCheck) {
            if (bodyTarget.x > (Vars::screenCenter.x + Vars::aimbotFov)) return false;
            if (bodyTarget.x < (Vars::screenCenter.x - Vars::aimbotFov)) return false;
            if (bodyTarget.y > (Vars::screenCenter.y + Vars::aimbotFov)) return false;
            if (bodyTarget.y < (Vars::screenCenter.y - Vars::aimbotFov)) return false;
        }

        // Check if the aimbot key is pressed
        if (GetAsyncKeyState(Vars::aimkey)) {
            // Set the target player if not already set
            if (Vars::targetPlayer == nullptr) {
                Vars::targetPlayer = target;
            }
            // Move the mouse to the target if it's the current target
            if (Vars::targetPlayer == target) {
                MouseMove(bodyTarget.x, bodyTarget.y, Vars::screenSize.x, Vars::screenSize.y, Vars::smooth);
            }
            else {
                Vars::targetPlayer = nullptr;
            }
            return true;
        }
        return false;
    }
    // SOURCE: https://www.unknowncheats.me/forum/unity/623384-introduction-il2cpp-api.html
    // Implemented this function based on this source
    // IMPORTANT, as the offset of the camera changes somehow every time the game is started. With this function we can get the offset dynamically
    bool FindOffsets()
    {
        // Find the UnityEngine.Camera class using IL2CPP API
        Unity::il2cppClass* CameraClass = IL2CPP::Class::Find("UnityEngine.Camera");
        // Get the method pointer for the set_fieldOfView function and store it in Offsets
        Offsets::setFieldOfView = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(CameraClass, "set_fieldOfView");
        // Return true to indicate successful offset finding
        return true;
    }
    // SOURCE: https://github.com/k-i-o/IL2CPPBaseByKio
    // Modified: added the player search part
    bool PlayerCache()
    {
        while (true)
        {
            // Attach the current thread to the IL2CPP domain
            void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
            Vars::localPlayer = NULL;
            Vars::playerList.clear();

            // Find all objects of type "UnityEngine.Rigidbody"
            auto list = Unity::Object::FindObjectsOfType<Unity::CComponent>("UnityEngine.Rigidbody");
            for (int i = 0; i < list->m_uMaxLength; i++)
            {
                if (!list->operator[](i)) continue;
                auto obj = list->operator[](i);
                if (!obj) continue;

                std::string objectname = obj->GetName()->ToString();
                // Check if the object name starts with "[Player:"
                if (objectname.find("[Player:") == 0)
                {
                    Vars::playerList.push_back(list->operator[](i)->GetGameObject());
                }
            }

            // Detach the thread from the IL2CPP domain
            IL2CPP::Thread::Detach(m_pThisThread);
            Sleep(1000);  // Sleep for 1 second before next iteration
        }
    }
    // SOURCE: https://www.codeproject.com/Articles/5051/Various-methods-for-capturing-the-screen
    // SOURCE: https://stackoverflow.com/questions/10623787/directx-11-framebuffer-capture-c-no-win32-or-d3dx
    // SOURCE: https://www.unknowncheats.me/forum/general-programming-and-reversing/422635-fastest-method-capture-screen.html
    // SOURCE: https://github.com/obsproject/obs-studio/tree/master/libobs-winrt
    // Implemented this function based on these sources
    cv::Mat CaptureFrameGPU(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
    {
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr))
        {
            std::cerr << "Error retrieving the back buffer." << std::endl;
            return cv::Mat();
        }

        // Create a staging texture for CPU access
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
            std::cerr << "Error creating the staging texture." << std::endl;
            backBuffer->Release();
            return cv::Mat();
        }

        // Copy the back buffer to the staging texture
        context->CopyResource(stagingTexture, backBuffer);
        backBuffer->Release();

        // Map the staging texture to access the image data
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr))
        {
            std::cerr << "Error mapping the staging texture." << std::endl;
            stagingTexture->Release();
            return cv::Mat();
        }

        // Create an OpenCV Mat object from the image data
        cv::Mat image(desc.Height, desc.Width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);
        cv::Mat result;
        cv::cvtColor(image, result, cv::COLOR_BGRA2BGR);

        // Unmap the staging texture
        context->Unmap(stagingTexture, 0);
        stagingTexture->Release();

        return result;
    }
    // Own contribution
    void InitializeAISettings() 
    {
        for (int i = 0; i < IM_ARRAYSIZE(AI::classNames); i++) 
        {
            AI::classSelected[i] = (std::find(AI::classesToShow.begin(), AI::classesToShow.end(), AI::classNames[i]) != AI::classesToShow.end());
        }
    }
    // SOURCE: https://niemand.com.ar/2019/01/13/creating-your-own-wallhack/
    // Modified: for the playermodel and case
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
    // Own contribution
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
            // Initialize AI model if not already done
            if (!AI::initAI)
            {
                try
                {
                    // Create new Inference object with specified model path and CUDA support
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

            // Capture current frame from the game
            cv::Mat frame = Functions::CaptureFrameGPU(Vars::pDevice, Vars::pContext, Vars::pSwapChain);
            if (!frame.empty())
            {
                std::vector<YOLO::Detection> output;
                if (AI::enableFrameLimit)
                {
                    // Frame limiting: Only perform inference every X frames
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
                    // Perform inference on every frame if frame limiting is disabled
                    output = AI::inferenceModel->runInference(frame);
                }

                // Draw bounding boxes for detected objects
                YOLO::DrawBoundingBoxes(output, AI::classesToShow);

                if (!output.empty())
                {
                    // Update screen size for extended AI functions
                    ExtendedAI::screenSizeX = Vars::screenSize.x;
                    ExtendedAI::screenSizeY = Vars::screenSize.y;

                    // Prioritize detected body parts
                    auto prioTargets = ExtendedAI::PrioritizeBodyParts(output, AI::classesToShow);
                    if (!prioTargets.empty())
                    {
                        const auto& target = prioTargets[0];
                        // Calculate target point (center of bounding box)
                        cv::Point2f currentTargetPos(
                            target.box.x + target.box.width / 2.0f,
                            target.box.y + target.box.height / 2.0f
                        );

                        // Current aim point (screen center)
                        static cv::Point2f currentAimPoint(Vars::screenSize.x / 2, Vars::screenSize.y / 2);
                        // Calculate new aim point with simulated human behavior
                        cv::Point2f newAimPoint = ExtendedAI::SimulateHumanAiming(currentAimPoint, currentTargetPos, ExtendedAI::aimSpeed);

                        // Auto-aim or aim when Alt key is pressed
                        if (AI::autoAim || (GetAsyncKeyState(VK_MENU) & 0x8000))
                        {
                            Functions::MouseMove(newAimPoint.x, newAimPoint.y, Vars::screenSize.x, Vars::screenSize.y, 1);
                            currentAimPoint = newAimPoint;
                        }
                    }
                }
            }
            // Reinitialize model if path has changed
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
