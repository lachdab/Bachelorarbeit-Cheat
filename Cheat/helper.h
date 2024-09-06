#pragma once
#include "il2cpp.h"
#include <cstdint>
#include <d3d11.h>
#include <IL2CPP_Resolver/Unity/Structures/Engine.hpp>

namespace Offsets
{
	uintptr_t setFieldOfView = 0x0;
}

namespace Vars
{
	ID3D11Device* pDevice = NULL;
	ID3D11DeviceContext* pContext = NULL;
	IDXGISwapChain* pSwapChain = NULL;

	uintptr_t Base;
	uintptr_t GameAssembly;
	uintptr_t UnityPlayer;

	bool firstTime = true;
	bool init = false;
	inline static UINT vps = 1;
	Unity::Vector2 screenSize = { 0, 0 };
	Unity::Vector2 screenCenter = { 0, 0 };
	D3D11_VIEWPORT viewport;
	bool drawIndexedInstanced = false;

	// ImGUI
	bool showImGuiMenu = false;
	const char* keyNames[] = { "Alt", "C", "V", "Right Mouse Button" };
	DWORD keyValues[] = { VK_MENU, 'C', 'V', VK_RBUTTON };
	static int selectedKeyIndex = 0;
	static int tab = 0;

	// Wallhack
	// Booleans for different wallhack models
	bool wallhack = false;
	bool cases = false;
	bool shader = false;

	// Aimbot
	float smooth = 1;
	float aimbotFov = 250.0f;
	bool aimbot = false;
	bool fovCheck = false;
	DWORD aimkey = VK_MENU;

	// Camera
	float cameraFOV = 120.0f;
	bool fovChanger = false;

	// Player
	Unity::CGameObject* localPlayer = NULL;
	std::vector<Unity::CGameObject*> playerList(NULL);
	Unity::CGameObject* targetPlayer = NULL;
	Unity::Vector3 playerPos = { 0, 0, 0 };
	const char* bones[]{ "Head", "Chest"};
	const char* marineHeadPath = "VisualsRoot/Sci_Fi_Character_08_05/root/pelvis/spine_01/spine_02/spine_03/neck_01/head";
	const char* marineChestPath = "VisualsRoot/Sci_Fi_Character_08_05/root/pelvis/spine_01/spine_02/spine_03";
	const char* soldierHeadPath = "VisualsRoot/Sci_Fi_Character_08_03/root/pelvis/spine_01/spine_02/spine_03/neck_01/head";
	const char* soldierChestPath = "VisualsRoot/Sci_Fi_Character_08_03/root/pelvis/spine_01/spine_02/spine_03";
	int boneSelected = 0;
}

namespace AI
{
	bool initAI = false;
	bool enableAIAimbot = false;
	bool enableCuda = true;
	bool enableFrameLimit = false;
	int inferenceInterval = 5;
	std::vector<std::string> classesToShow = { "Head" };
	const char* classNames[] = { "Back", "Body", "Head", "Left-Leg", "Player", "Right-Leg"};
	std::vector<bool> classSelected(IM_ARRAYSIZE(classNames), false);
	bool pathChanged = false;
	std::string modelPath = "F:\\Bachelorarbeit\\BuildGame2.0\\ki_aimbot_v3_300E.onnx";
	bool autoAim = false;
	YOLO::Inference* inferenceModel = nullptr;
	int frameCounter = 0;
}