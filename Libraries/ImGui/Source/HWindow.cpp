#include "HWindow.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <Windows.h>
#include <tchar.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of helper functions
void CleanupDeviceD3D();
void WaitForLastSubmittedFrameBackend();

namespace
{
	HDirectXContext DirectXContext{};

	HWND MainWindowHandle = 0;

	union HGUIWindowExtra
	{
		struct
		{
			uint32_t ByteA;
			uint32_t ByteB;
		};
		HGUIWindow* GUIWindow;
	};

	// Win32 message handler
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
	// dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your
	// main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
	// your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from
	// your application based on those two flags.
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
			case WM_SIZE:
				if (DirectXContext.Device != NULL && wParam != SIZE_MINIMIZED && MainWindowHandle == hWnd)
				{
					WaitForLastSubmittedFrameBackend();

					HGUIWindowExtra GUIWindowExtra;
					GUIWindowExtra.ByteA = GetWindowLongA(hWnd, 0 * sizeof(uint32_t));
					GUIWindowExtra.ByteB = GetWindowLongA(hWnd, 1 * sizeof(uint32_t));

					HImGui::DestroyRenderTargets(*GUIWindowExtra.GUIWindow);

					HRESULT result = GUIWindowExtra.GUIWindow->SwapChain.SwapChain->ResizeBuffers(
						0,
						(UINT)LOWORD(lParam),
						(UINT)HIWORD(lParam),
						DXGI_FORMAT_UNKNOWN,
						DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
					assert(SUCCEEDED(result) && "Failed to resize swapchain.");
					HImGui::CreateRenderTargets(*GUIWindowExtra.GUIWindow);
				}
				return 0;
			case WM_SYSCOMMAND:
				if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
					return 0;
				break;
			case WM_DESTROY:
				::PostQuitMessage(0);
				return 0;
		}
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}
} // namespace

bool HImGui::CreateGUIWindow(HGUIWindow* GUIWindow)
{

	// Create application window
	// ImGui_ImplWin32_EnableDpiAwareness();
	GUIWindow->WindowClass.cbSize = sizeof(WNDCLASSEX);
	GUIWindow->WindowClass.style = CS_CLASSDC;
	GUIWindow->WindowClass.lpfnWndProc = WndProc;
	GUIWindow->WindowClass.cbClsExtra = 0;
	GUIWindow->WindowClass.cbWndExtra = sizeof(HGUIWindowExtra);
	GUIWindow->WindowClass.hInstance = GetModuleHandle(NULL);
	GUIWindow->WindowClass.hIcon = NULL;
	GUIWindow->WindowClass.hCursor = NULL;
	GUIWindow->WindowClass.hbrBackground = NULL;
	GUIWindow->WindowClass.lpszMenuName = NULL;
	GUIWindow->WindowClass.lpszClassName = _T("Honey");
	GUIWindow->WindowClass.hIconSm = NULL;
	::RegisterClassEx(&GUIWindow->WindowClass);

	GUIWindow->WindowHandle = CreateWindow(
		GUIWindow->WindowClass.lpszClassName,
		_T("Dear ImGui DirectX12 Example"),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		GUIWindow->WindowClass.hInstance,
		NULL);

	HGUIWindowExtra GUIWindowExtra;
	GUIWindowExtra.GUIWindow = GUIWindow;
	SetWindowLongA(GUIWindow->WindowHandle, 0 * sizeof(uint32_t), GUIWindowExtra.ByteA);
	SetWindowLongA(GUIWindow->WindowHandle, 1 * sizeof(uint32_t), GUIWindowExtra.ByteB);
	MainWindowHandle = GUIWindow->WindowHandle;

	GUIWindow->DirectXContext = &DirectXContext;

	// Initialize Direct3D
	if (!HDirectX::CreateDeviceD3D(&GUIWindow->DirectXContext->Device, GUIWindow->WindowHandle))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	// RTV
	if (!HDirectX::CreateRTVHeap(
			&GUIWindow->DirectXContext->RTV_DescHeap,
			GUIWindow->DirectXContext->Device,
			HGUIWindow::NUM_BACK_BUFFERS))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}
	SIZE_T rtvDescriptorSize =
		GUIWindow->DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = DirectXContext.RTV_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < HGUIWindow::NUM_BACK_BUFFERS; i++)
	{
		GUIWindow->RenderTargetDescriptor[i] = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// CBVSRVUAV
	if (!HDirectX::CreateCBVSRVUAVHeap(&GUIWindow->DirectXContext->CBVSRVUAV_DescHeap, GUIWindow->DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	// Command Queue
	if (!HDirectX::CreateCommandQueue(&GUIWindow->DirectXContext->CommandQueue, GUIWindow->DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	// Command Allocators
	for (UINT i = 0; i < HDirectXContext::NUM_FRAMES_IN_FLIGHT; i++)
	{
		if (!HDirectX::CreateCommandAllocator(
				&DirectXContext.FrameContext[i].CommandAllocator,
				GUIWindow->DirectXContext->Device))
		{
			HImGui::DestroyGUIWindow(*GUIWindow);
			return false;
		}
	}

	// Command List
	if (!HDirectX::CreateCommandList(
			&DirectXContext.CommandList,
			DirectXContext.FrameContext[0].CommandAllocator,
			GUIWindow->DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	// Fence
	if (!HDirectX::CreateFence(DirectXContext.Fence, GUIWindow->DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	// Swap Chain
	if (!HDirectX::CreateSwapChain(
			GUIWindow->SwapChain,
			GUIWindow->WindowHandle,
			HGUIWindow::NUM_BACK_BUFFERS,
			DirectXContext.CommandQueue,
			DirectXContext.Device))
	{
		HImGui::DestroyGUIWindow(*GUIWindow);
		return false;
	}

	CreateRenderTargets(*GUIWindow);

	// Show the window
	ShowWindow(GUIWindow->WindowHandle, SW_SHOWDEFAULT);
	UpdateWindow(GUIWindow->WindowHandle);

	return true;
}

void HImGui::DestroyGUIWindow(HGUIWindow& GUIWindow)
{
	WaitForLastSubmittedFrameBackend();

	HImGui::DestroyRenderTargets(GUIWindow);

	if (GUIWindow.SwapChain.SwapChain != nullptr)
	{
		GUIWindow.SwapChain.SwapChain->SetFullscreenState(false, NULL);
		GUIWindow.SwapChain.SwapChain->Release();
		GUIWindow.SwapChain.SwapChain = NULL;
	}
	if (GUIWindow.SwapChain.SwapChainWaitableObject != nullptr)
	{
		CloseHandle(GUIWindow.SwapChain.SwapChainWaitableObject);
	}

	CleanupDeviceD3D();
	::DestroyWindow(GUIWindow.WindowHandle);
	::UnregisterClass(GUIWindow.WindowClass.lpszClassName, GUIWindow.WindowClass.hInstance);
}

HFrameContext* HImGui::WaitForNextFrameResources(HGUIWindow& GUIWindow)
{
	UINT nextFrameIndex = DirectXContext.FrameIndex + 1;
	DirectXContext.FrameIndex = nextFrameIndex;

	HANDLE waitableObjects[] = { GUIWindow.SwapChain.SwapChainWaitableObject, NULL };
	DWORD numWaitableObjects = 1;

	HFrameContext* frameCtx = &DirectXContext.FrameContext[nextFrameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];
	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue != 0) // means no fence was signaled
	{
		frameCtx->FenceValue = 0;
		DirectXContext.Fence.Fence->SetEventOnCompletion(fenceValue, DirectXContext.Fence.FenceEvent);
		waitableObjects[1] = DirectXContext.Fence.FenceEvent;
		numWaitableObjects = 2;
	}

	WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

	return frameCtx;
}

void HImGui::WaitForLastSubmittedFrame()
{
	WaitForLastSubmittedFrameBackend();
}

// Helper functions

void CleanupDeviceD3D()
{
	for (UINT i = 0; i < HDirectXContext::NUM_FRAMES_IN_FLIGHT; i++)
	{
		if (DirectXContext.FrameContext[i].CommandAllocator)
		{
			DirectXContext.FrameContext[i].CommandAllocator->Release();
			DirectXContext.FrameContext[i].CommandAllocator = NULL;
		}
	}

	if (DirectXContext.CommandQueue)
	{
		DirectXContext.CommandQueue->Release();
		DirectXContext.CommandQueue = NULL;
	}
	if (DirectXContext.CommandList)
	{
		DirectXContext.CommandList->Release();
		DirectXContext.CommandList = NULL;
	}
	if (DirectXContext.RTV_DescHeap)
	{
		DirectXContext.RTV_DescHeap->Release();
		DirectXContext.RTV_DescHeap = NULL;
	}
	if (DirectXContext.CBVSRVUAV_DescHeap)
	{
		DirectXContext.CBVSRVUAV_DescHeap->Release();
		DirectXContext.CBVSRVUAV_DescHeap = NULL;
	}

	DirectXContext.Fence.Release();

	if (DirectXContext.Device)
	{
		DirectXContext.Device->Release();
		DirectXContext.Device = NULL;
	}

#ifdef DX12_ENABLE_DEBUG_LAYER
	IDXGIDebug1* pDebug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif
}

void HImGui::CreateRenderTargets(HGUIWindow& GUIWindow)
{
	for (UINT i = 0; i < HGUIWindow::NUM_BACK_BUFFERS; i++)
	{
		ID3D12Resource* pBackBuffer = NULL;
		GUIWindow.SwapChain.SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		DirectXContext.Device->CreateRenderTargetView(pBackBuffer, NULL, GUIWindow.RenderTargetDescriptor[i]);
		GUIWindow.RenderTargetResource[i] = pBackBuffer;
	}
}


void HImGui::DestroyRenderTargets(HGUIWindow& GUIWindow)
{
	for (UINT i = 0; i < HGUIWindow::NUM_BACK_BUFFERS; i++)
	{
		if (GUIWindow.RenderTargetResource[i])
		{
			GUIWindow.RenderTargetResource[i]->Release();
			GUIWindow.RenderTargetResource[i] = nullptr;
		}
	}
}

void WaitForLastSubmittedFrameBackend()
{
	HFrameContext* frameCtx =
		&DirectXContext.FrameContext[DirectXContext.FrameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];

	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue == 0)
		return; // No fence was signaled

	frameCtx->FenceValue = 0;
	if (DirectXContext.Fence.Fence->GetCompletedValue() >= fenceValue)
		return;

	DirectXContext.Fence.Fence->SetEventOnCompletion(fenceValue, DirectXContext.Fence.FenceEvent);
	WaitForSingleObject(DirectXContext.Fence.FenceEvent, INFINITE);
}