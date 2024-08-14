#pragma once
#include <d3d11.h>
#include <cstdio>

namespace gui
{
	extern HINSTANCE dll_handle;

	// helper functions
	extern HRESULT GenerateShader(ID3D11PixelShader** pShader, float r, float g, float b);

	// main function
	int WINAPI RunGUI();

	// hook functions
	LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static long __stdcall hkPresent(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags);
	bool GetPresentPointer();
	DWORD __stdcall EjectThread(LPVOID lpParameter);
}
