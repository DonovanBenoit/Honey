#include "HWindow.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <Windows.h>
#include <tchar.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of helper functions
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrameBackend();

namespace
{
	HDirectXContext DirectXContext{};

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
				if (DirectXContext.g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
				{
					WaitForLastSubmittedFrameBackend();
					CleanupRenderTarget();
					HRESULT result = DirectXContext.SwapChain.SwapChain->ResizeBuffers(
						0,
						(UINT)LOWORD(lParam),
						(UINT)HIWORD(lParam),
						DXGI_FORMAT_UNKNOWN,
						DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
					assert(SUCCEEDED(result) && "Failed to resize swapchain.");
					CreateRenderTarget();
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

bool HImGui::CreateGUIWindow(HGUIWindow& GUIWindow)
{
	// Create application window
	// ImGui_ImplWin32_EnableDpiAwareness();
	GUIWindow.WindowClass = { sizeof(WNDCLASSEX),	 CS_CLASSDC, WndProc, 0L,	0L,
							  GetModuleHandle(NULL), NULL,		 NULL,	  NULL, NULL,
							  _T("ImGui Example"),	 NULL };
	::RegisterClassEx(&GUIWindow.WindowClass);

	GUIWindow.WindowHandle = ::CreateWindow(
		GUIWindow.WindowClass.lpszClassName,
		_T("Dear ImGui DirectX12 Example"),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		GUIWindow.WindowClass.hInstance,
		NULL);

	GUIWindow.DirectXContext = &DirectXContext;

	// Initialize Direct3D
	if (!HDirectX::CreateDeviceD3D(&GUIWindow.DirectXContext->g_pd3dDevice, GUIWindow.WindowHandle))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// RTV
	if (!HDirectX::CreateRTVHeap(&GUIWindow.DirectXContext->g_pd3dRtvDescHeap, GUIWindow.DirectXContext->g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}
	SIZE_T rtvDescriptorSize =
		GUIWindow.DirectXContext->g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = DirectXContext.g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < HDirectXContext::NUM_BACK_BUFFERS; i++)
	{
		DirectXContext.g_mainRenderTargetDescriptor[i] = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// CBVSRVUAV
	if (!HDirectX::CreateCBVSRVUAVHeap(
			&GUIWindow.DirectXContext->g_pd3dSrvDescHeap,
			GUIWindow.DirectXContext->g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Command Queue
	if (!HDirectX::CreateCommandQueue(
			&GUIWindow.DirectXContext->CommandQueue,
			GUIWindow.DirectXContext->g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Command Allocators
	for (UINT i = 0; i < HDirectXContext::NUM_FRAMES_IN_FLIGHT; i++)
	{
		if (!HDirectX::CreateCommandAllocator(
				&DirectXContext.g_frameContext[i].CommandAllocator,
				GUIWindow.DirectXContext->g_pd3dDevice))
		{
			CleanupDeviceD3D();
			HImGui::DestroyGUIWindow(GUIWindow);
			return false;
		}
	}

	// Command List
	if (!HDirectX::CreateCommandList(
			&DirectXContext.CommandList,
			DirectXContext.g_frameContext[0].CommandAllocator,
			GUIWindow.DirectXContext->g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Fence
	if (!HDirectX::CreateFence(
		DirectXContext.Fence,
		GUIWindow.DirectXContext->g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Swap Chain
	if (!HDirectX::CreateSwapChain(DirectXContext.SwapChain, GUIWindow.WindowHandle, DirectXContext.CommandQueue, DirectXContext.g_pd3dDevice))
	{
		CleanupDeviceD3D();
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	CreateRenderTarget();

	// Show the window
	ShowWindow(GUIWindow.WindowHandle, SW_SHOWDEFAULT);
	UpdateWindow(GUIWindow.WindowHandle);

	return true;
}

void HImGui::DestroyGUIWindow(HGUIWindow& GUIWindow)
{
	CleanupDeviceD3D();
	::DestroyWindow(GUIWindow.WindowHandle);
	::UnregisterClass(GUIWindow.WindowClass.lpszClassName, GUIWindow.WindowClass.hInstance);
}

HFrameContext* HImGui::WaitForNextFrameResources(HGUIWindow& GUIWindow)
{
	UINT nextFrameIndex = DirectXContext.g_frameIndex + 1;
	DirectXContext.g_frameIndex = nextFrameIndex;

	HANDLE waitableObjects[] = { DirectXContext.SwapChain.SwapChainWaitableObject, NULL };
	DWORD numWaitableObjects = 1;

	HFrameContext* frameCtx = &DirectXContext.g_frameContext[nextFrameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];
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
	CleanupRenderTarget();
	if (DirectXContext.SwapChain.SwapChain != nullptr)
	{
		DirectXContext.SwapChain.SwapChain->SetFullscreenState(false, NULL);
		DirectXContext.SwapChain.SwapChain->Release();
		DirectXContext.SwapChain.SwapChain = NULL;
	}
	if (DirectXContext.SwapChain.SwapChainWaitableObject != nullptr)
	{
		CloseHandle(DirectXContext.SwapChain.SwapChainWaitableObject);
	}
	for (UINT i = 0; i < HDirectXContext::NUM_FRAMES_IN_FLIGHT; i++)
		if (DirectXContext.g_frameContext[i].CommandAllocator)
		{
			DirectXContext.g_frameContext[i].CommandAllocator->Release();
			DirectXContext.g_frameContext[i].CommandAllocator = NULL;
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
	if (DirectXContext.g_pd3dRtvDescHeap)
	{
		DirectXContext.g_pd3dRtvDescHeap->Release();
		DirectXContext.g_pd3dRtvDescHeap = NULL;
	}
	if (DirectXContext.g_pd3dSrvDescHeap)
	{
		DirectXContext.g_pd3dSrvDescHeap->Release();
		DirectXContext.g_pd3dSrvDescHeap = NULL;
	}
	
	DirectXContext.Fence.Release();

	if (DirectXContext.g_pd3dDevice)
	{
		DirectXContext.g_pd3dDevice->Release();
		DirectXContext.g_pd3dDevice = NULL;
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

void CreateRenderTarget()
{
	for (UINT i = 0; i < HDirectXContext::NUM_BACK_BUFFERS; i++)
	{
		ID3D12Resource* pBackBuffer = NULL;
		DirectXContext.SwapChain.SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		DirectXContext.g_pd3dDevice->CreateRenderTargetView(
			pBackBuffer,
			NULL,
			DirectXContext.g_mainRenderTargetDescriptor[i]);
		DirectXContext.g_mainRenderTargetResource[i] = pBackBuffer;
	}
}

void CleanupRenderTarget()
{
	WaitForLastSubmittedFrameBackend();

	for (UINT i = 0; i < HDirectXContext::NUM_BACK_BUFFERS; i++)
		if (DirectXContext.g_mainRenderTargetResource[i])
		{
			DirectXContext.g_mainRenderTargetResource[i]->Release();
			DirectXContext.g_mainRenderTargetResource[i] = NULL;
		}
}

void WaitForLastSubmittedFrameBackend()
{
	HFrameContext* frameCtx =
		&DirectXContext.g_frameContext[DirectXContext.g_frameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];

	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue == 0)
		return; // No fence was signaled

	frameCtx->FenceValue = 0;
	if (DirectXContext.Fence.Fence->GetCompletedValue() >= fenceValue)
		return;

	DirectXContext.Fence.Fence->SetEventOnCompletion(fenceValue, DirectXContext.Fence.FenceEvent);
	WaitForSingleObject(DirectXContext.Fence.FenceEvent, INFINITE);
}