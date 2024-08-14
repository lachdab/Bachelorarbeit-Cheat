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
    void ImGuiCustomStyle() {
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
    HWND GetHandlerByWindowTitle(const std::wstring& windowTitle) {
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
    bool WorldToScreen(Unity::Vector3 world, Unity::Vector2& screen)
    {
        Unity::CCamera* CameraMain = Unity::Camera::GetMain();
        if (!CameraMain) {
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

        if (GetAsyncKeyState(Vars::aimkey)) {
            if (Vars::targetPlayer == nullptr) {
                Vars::targetPlayer = target;
            }

            if (Vars::targetPlayer == target) {
                MouseMove(bodyTarget.x, bodyTarget.y, Vars::screenSize.x, Vars::screenSize.y, Vars::smooth);
            }
        }
        else {
            Vars::targetPlayer = nullptr;
        }
        return true;
    }
#pragma endregion
}
