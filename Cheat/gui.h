#pragma once
#include <d3d11.h>
#include <thread>

namespace gui
{
	// main function
	int RunGUI();

	// Forward declarations of helper functions
	bool CreateDeviceD3D(HWND hWnd);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND getHandlerByWindowTitle(const std::wstring& windowTitle);

	// Debug and Logging
	std::string GetErrorMessage(HRESULT hr);
}
