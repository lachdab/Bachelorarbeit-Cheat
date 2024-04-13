#include "gui_OLD.h"

#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_dx11.h"
#include "ext/imgui/imgui_impl_win32.h"

// ImGui Processhandler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);

// Define render target view and depth stencil view
ID3D11RenderTargetView* gui_old::renderTargetView = nullptr;
ID3D11DepthStencilView* gui_old::depthStencilView = nullptr;

// Own Processhandler
// handles all the events that windows sends to the window
LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter)) return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui_old::swapChain && wideParameter != SIZE_MINIMIZED)
		{
			// Important to avoid resource leaks
			ImGui_ImplDX11_InvalidateDeviceObjects();

			DXGI_SWAP_CHAIN_DESC sd;
			gui_old::swapChain->GetDesc(&sd);
			sd.BufferDesc.Width = LOWORD(longParameter);
			sd.BufferDesc.Height = HIWORD(longParameter);
			sd.BufferDesc.RefreshRate.Numerator = 0;
			sd.BufferDesc.RefreshRate.Denominator = 1;

			gui_old::swapChain->ResizeBuffers(sd.BufferCount, sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format, 0);

			//gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			//gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			//gui::ResetDevice();
			ImGui_ImplDX11_CreateDeviceObjects();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui_old::position = MAKEPOINTS(longParameter); //set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{};

			GetWindowRect(gui_old::window, &rect);

			rect.left += points.x - gui_old::position.x;
			rect.right += points.y - gui_old::position.y;

			if (gui_old::position.x >= 0 && gui_old::position.x <= gui_old::WIDTH && gui_old::position.y >= 0 && gui_old::position.y <= 19)
				SetWindowPos(gui_old::window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
		}
	}
	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}


void gui_old::CreateHWindow(const char* windowName, const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(className, windowName, WS_POPUP, 100, 100, WIDTH, HEIGHT, 0, 0, windowClass.hInstance, 0);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui_old::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}


bool gui_old::CreateDevice() noexcept
{
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);

	if (FAILED(hr))
		return false;

	// Get the swap chain
	DXGI_SWAP_CHAIN_DESC scDesc = {};
	scDesc.BufferCount = 1;
	scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = window;
	scDesc.SampleDesc.Count = 1;
	scDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
		nullptr, 0, D3D11_SDK_VERSION, &scDesc,
		&swapChain, &device, nullptr, &context
	);

	if (FAILED(hr))
		return false;

	return true;

	/*d3d = Direct3DCreate9(D3D_SDK_VERSION);
	
	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &device) < 0)
		return false;

	return true;*/
}

void gui_old::ResetDevice() noexcept
{
	// not needed in DirectX 11 so maybe remove ResetDevice later
	ImGui_ImplDX11_InvalidateDeviceObjects();

	// Recreate device resources
	ImGui_ImplDX11_CreateDeviceObjects();
}

void gui_old::DestroyDevice() noexcept
{
	if (context) {
		context->Release();
		context = nullptr;
	}

	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}

	if (device) {
		device->Release();
		device = nullptr;
	}
	/*if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}*/
}

void gui_old::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, context);
}

void gui_old::DestroyImGui() noexcept
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui_old::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Start the ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui_old::EndRender() noexcept
{
	ImGui::EndFrame();

	// Set render states
	context->OMSetDepthStencilState(nullptr, 0);
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	context->RSSetScissorRects(0, nullptr);

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);

	// Clear the render target and depth stencil buffer
	//context->ClearRenderTargetView(renderTargetView, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Rendering
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 1, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->OMSetRenderTargets(1, &renderTargetView, nullptr);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the frame
	swapChain->Present(0, 0);

	// Handle device lost
	HRESULT hr = device->GetDeviceRemovedReason();
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		ResetDevice();
	}

	/*device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
		*/
}

void gui_old::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin("Trainer", &exit, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

	ImGui::Button("Money");

	ImGui::End();
}