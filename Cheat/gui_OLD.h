#pragma once
//#include <d3d9.h>
// DirectX11
#include <d3d11.h>

namespace gui_old
{
	// window size
	constexpr int WIDTH = 500;
	constexpr int HEIGHT = 300;

	inline bool exit = true;

	// winapi window vars (Window Information)
	inline HWND window = nullptr;
	inline WNDCLASSEXA windowClass = {};

	// points for window movement
	inline POINTS position = {};

	// direct x state vars
	inline ID3D11Device* device = nullptr;
	inline ID3D11DeviceContext* context = nullptr;
	inline IDXGISwapChain* swapChain = nullptr;

	// Declare render target view and depth stencil view
	extern ID3D11RenderTargetView* renderTargetView;
	extern ID3D11DepthStencilView* depthStencilView;

	// handle window creation & destruction
	void CreateHWindow(const char* windowName, const char* className) noexcept;
	void DestroyHWindow() noexcept;

	// handle device creation & destruction
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	// handle ImGui creation & destruction
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
}