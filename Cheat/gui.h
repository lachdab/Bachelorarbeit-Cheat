#pragma once
#include <d3d11.h>
#include <thread>

namespace gui
{
	extern HINSTANCE dll_handle;

	// helper functions
	HWND getHandlerByWindowTitle(const std::wstring& windowTitle);
	extern HRESULT GenerateShader(ID3D11PixelShader** pShader, float r, float g, float b);

	// main function
	int WINAPI RunGUI();

	// imgui style function
	void ImGuiCustomStyle();

	// hook functions
	LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags);
	bool get_present_pointer();
	DWORD __stdcall EjectThread(LPVOID lpParameter);
}
