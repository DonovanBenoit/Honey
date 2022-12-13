#include "HWindow.h"

#include "HImGui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <Windows.h>
#include <directx/d3dx12.h>
#include <tchar.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of helper functions
void CleanupDeviceD3D();
void WaitForLastSubmittedFrameBackend();

namespace
{
	HDirectXContext DirectXContext{};

	union HGUIWindowExtra
	{
		HGUIWindow* GUIWindow;
		struct
		{
			uint32_t ByteA;
			uint32_t ByteB;
		};
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
	LRESULT WINAPI WndProc(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT ImGuiWndProcResult = ImGui_ImplWin32_WndProcHandler(WindowHandle, Message, wParam, lParam);

		if (ImGui::GetCurrentContext() != nullptr)
		{
			ImGuiIO& IO = ImGui::GetIO();
			if (IO.WantCaptureMouse || IO.WantCaptureKeyboard)
			{
				return TRUE;
			}
		}

		switch (Message)
		{
			case WM_SIZE:
				if (DirectXContext.Device != NULL && wParam != SIZE_MINIMIZED)
				{
					WaitForLastSubmittedFrameBackend();

					HGUIWindowExtra GUIWindowExtra;
					GUIWindowExtra.ByteA = GetWindowLongA(WindowHandle, 0 * sizeof(uint32_t));
					GUIWindowExtra.ByteB = GetWindowLongA(WindowHandle, 1 * sizeof(uint32_t));

					HImGui::DestroyRenderTargets(*GUIWindowExtra.GUIWindow);

					HRESULT Result = GUIWindowExtra.GUIWindow->SwapChain.SwapChain->ResizeBuffers(
						0,
						(UINT)LOWORD(lParam),
						(UINT)HIWORD(lParam),
						DXGI_FORMAT_UNKNOWN,
						DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
					assert(SUCCEEDED(Result) && "Failed to resize swapchain.");
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
		return ::DefWindowProc(WindowHandle, Message, wParam, lParam);
	}
} // namespace

bool HImGui::CreateGUIWindow(HGUIWindow& GUIWindow)
{
	// Create application window
	// ImGui_ImplWin32_EnableDpiAwareness();
	GUIWindow.WindowClass.cbSize = sizeof(WNDCLASSEX);
	GUIWindow.WindowClass.style = CS_CLASSDC;
	GUIWindow.WindowClass.lpfnWndProc = WndProc;
	GUIWindow.WindowClass.cbClsExtra = 0;
	GUIWindow.WindowClass.cbWndExtra = sizeof(HGUIWindowExtra);
	GUIWindow.WindowClass.hInstance = GetModuleHandle(NULL);
	GUIWindow.WindowClass.hIcon = NULL;
	GUIWindow.WindowClass.hCursor = NULL;
	GUIWindow.WindowClass.hbrBackground = NULL;
	GUIWindow.WindowClass.lpszMenuName = NULL;
	GUIWindow.WindowClass.lpszClassName = _T("Honey");
	GUIWindow.WindowClass.hIconSm = NULL;
	::RegisterClassEx(&GUIWindow.WindowClass);

	GUIWindow.WindowHandle = CreateWindow(
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

	HGUIWindowExtra GUIWindowExtra = { &GUIWindow };
	SetWindowLongA(GUIWindow.WindowHandle, 0 * sizeof(uint32_t), GUIWindowExtra.ByteA);
	SetWindowLongA(GUIWindow.WindowHandle, 1 * sizeof(uint32_t), GUIWindowExtra.ByteB);

	GUIWindow.DirectXContext = &DirectXContext;

	// Initialize Direct3D
	if (!HDirectX::CreateDeviceD3D(&GUIWindow.DirectXContext->Device, GUIWindow.WindowHandle))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// RTV
	if (!HDirectX::CreateRTVHeap(
			&GUIWindow.RTV_DescHeap,
			GUIWindow.DirectXContext->Device,
			HGUIWindow::NUM_BACK_BUFFERS))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}
	SIZE_T rtvDescriptorSize =
		GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GUIWindow.RTV_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < HGUIWindow::NUM_BACK_BUFFERS; i++)
	{
		GUIWindow.RenderTargetDescriptor[i] = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// CBVSRVUAV
	if (!HDirectX::CreateCBVSRVUAVHeap(&GUIWindow.CBVSRVUAV_DescHeap, GUIWindow.DirectXContext->Device, 1024))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Command Queue
	if (!HDirectX::CreateCommandQueue(
			&GUIWindow.DirectXContext->CommandQueue,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Command Allocators
	for (UINT i = 0; i < HDirectXContext::NUM_FRAMES_IN_FLIGHT; i++)
	{
		if (!HDirectX::CreateCommandAllocator(
				&DirectXContext.FrameContext[i].CommandAllocator,
				GUIWindow.DirectXContext->Device,
				D3D12_COMMAND_LIST_TYPE_DIRECT))
		{
			HImGui::DestroyGUIWindow(GUIWindow);
			return false;
		}
	}

	// Command List
	if (!HDirectX::CreateCommandList(
			&DirectXContext.CommandList,
			DirectXContext.FrameContext[0].CommandAllocator,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Copy Command Queue
	if (!HDirectX::CreateCommandQueue(
			&GUIWindow.DirectXContext->CopyCommandQueue,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}
	// Copy Command Allocator
	if (!HDirectX::CreateCommandAllocator(
			&GUIWindow.DirectXContext->CopyCommandAllocator,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}
	// Copy Command List
	if (!HDirectX::CreateCommandList(
			&DirectXContext.CopyCommandList,
			GUIWindow.DirectXContext->CopyCommandAllocator,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}
	// Copy Fence
	if (!HDirectX::CreateFence(DirectXContext.CopyFence, GUIWindow.DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Fence
	if (!HDirectX::CreateFence(DirectXContext.Fence, GUIWindow.DirectXContext->Device))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	// Swap Chain
	if (!HDirectX::CreateSwapChain(
			GUIWindow.SwapChain,
			GUIWindow.WindowHandle,
			HGUIWindow::NUM_BACK_BUFFERS,
			DirectXContext.CommandQueue,
			DirectXContext.Device))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	CreateRenderTargets(GUIWindow);

	// Show the window
	ShowWindow(GUIWindow.WindowHandle, SW_SHOWDEFAULT);
	UpdateWindow(GUIWindow.WindowHandle);

	return true;
}

void HImGui::DestroyGUIWindow(HGUIWindow& GUIWindow)
{
	WaitForLastSubmittedFrameBackend();

	HImGui::DestroyRenderTargets(GUIWindow);

	if (GUIWindow.SwapChain.SwapChain != nullptr)
	{
		GUIWindow.SwapChain.SwapChain->SetFullscreenState(false, nullptr);
		GUIWindow.SwapChain.SwapChain->Release();
		GUIWindow.SwapChain.SwapChain = nullptr;
	}
	if (GUIWindow.SwapChain.SwapChainWaitableObject != nullptr)
	{
		CloseHandle(GUIWindow.SwapChain.SwapChainWaitableObject);
	}

	if (GUIWindow.RTV_DescHeap != nullptr)
	{
		GUIWindow.RTV_DescHeap->Release();
		GUIWindow.RTV_DescHeap = nullptr;
	}
	if (GUIWindow.CBVSRVUAV_DescHeap != nullptr)
	{
		GUIWindow.CBVSRVUAV_DescHeap->Release();
		GUIWindow.CBVSRVUAV_DescHeap = nullptr;
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
	HFrameContext* FrameContext =
		&DirectXContext.FrameContext[DirectXContext.FrameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];

	UINT64 FenceValue = FrameContext->FenceValue;
	if (FenceValue == 0)
		return; // No fence was signaled

	FrameContext->FenceValue = 0;

	HDirectX::WaitForFence(DirectXContext.Fence, FenceValue);
}

bool HImGui::CreateOrUpdateImage(HGUIWindow& GUIWindow, int64_t& ImageIndex, uint64_t Width, uint64_t Height)
{
	if (ImageIndex < 0)
	{
		if (GUIWindow.ImageRecycling.empty())
		{
			if (GUIWindow.Images.size() <= GUIWindow.ImageCount)
			{
				return false;
			}
			ImageIndex = GUIWindow.ImageCount;
			GUIWindow.ImageCount++;
		}
		else
		{
			ImageIndex = GUIWindow.ImageRecycling.back();
			GUIWindow.ImageRecycling.pop_back();
		}
	}

	HGUIImage& GUIImage = GUIWindow.Images[ImageIndex];

	const bool Resize = GUIImage.Width != Width || GUIImage.Height != Height;
	GUIImage.Width = Width;
	GUIImage.Height = Height;

	uint64_t GUIImageSize = GUIImage.Width * GUIImage.Height * sizeof(uint32_t);
	if (GUIImageSize == 0)
	{
		return false;
	}

	if (Resize)
	{
		if (GUIImage.Pixels != nullptr)
		{
			uint32_t* NewPixels = (uint32_t*)realloc(GUIImage.Pixels, GUIImageSize);
			if (NewPixels == nullptr)
			{
				free(GUIImage.Pixels);
			}
			GUIImage.Pixels = NewPixels;
		}
		else
		{
			GUIImage.Pixels = (uint32_t*)malloc(GUIImageSize);
		}
	}

	return true;
}

uint64_t CalculateAlignedSize(uint64_t Size, uint64_t Alignment)
{
	return Size + (Alignment - (Size % Alignment)) % Alignment;
}

bool HImGui::UploadImage(HGUIWindow& GUIWindow, int64_t ImageIndex)
{
	if (ImageIndex < 0 || ImageIndex >= GUIWindow.ImageCount)
	{
		return false;
	}

	HRESULT Result;

	static const D3D12_HEAP_PROPERTIES DefaultHeapProperties{ D3D12_HEAP_TYPE_DEFAULT,
															  D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
															  D3D12_MEMORY_POOL_UNKNOWN,
															  1,
															  1 };

	static const D3D12_HEAP_PROPERTIES UploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD,
															 D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
															 D3D12_MEMORY_POOL_UNKNOWN,
															 1,
															 1 };

	HGUIImage& GUIImage = GUIWindow.Images[ImageIndex];
	uint64_t SourceRowPitch = GUIImage.Width * 4;
	uint64_t AlignedRowPitch = CalculateAlignedSize(SourceRowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	if (GUIImage.Resource == nullptr)
	{
		// Texture Resource
		{
			D3D12_RESOURCE_DESC ResourceDesc{};
			ResourceDesc.DepthOrArraySize = 1;
			ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			ResourceDesc.Width = GUIImage.Width;
			ResourceDesc.Height = GUIImage.Height;
			ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			ResourceDesc.MipLevels = 1;
			ResourceDesc.SampleDesc.Count = 1;
			ResourceDesc.SampleDesc.Quality = 0;

			Result = GUIWindow.DirectXContext->Device->CreateCommittedResource(
				&DefaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&ResourceDesc,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				nullptr,
				IID_PPV_ARGS(&GUIImage.Resource));
		}

		// Upload Resource
		{
			D3D12_RESOURCE_DESC UploadResourceDesc{};
			UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			UploadResourceDesc.Alignment = 0;
			UploadResourceDesc.Width = AlignedRowPitch * GUIImage.Height;
			UploadResourceDesc.Height = 1;
			UploadResourceDesc.DepthOrArraySize = 1;
			UploadResourceDesc.MipLevels = 1;
			UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			UploadResourceDesc.SampleDesc.Count = 1;
			UploadResourceDesc.SampleDesc.Quality = 0;
			UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			UploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			Result = GUIWindow.DirectXContext->Device->CreateCommittedResource(
				&UploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&UploadResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&GUIImage.UploadResource));

			D3D12_RANGE UploadRange{ 0, AlignedRowPitch * GUIImage.Height };
			GUIImage.UploadResource->Map(0, &UploadRange, &GUIImage.UploadData);
		}

		SIZE_T CBVSRVUAV_DescriptorSize =
			GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE CBVSRVUAV_Handle =
			GUIWindow.CBVSRVUAV_DescHeap->GetCPUDescriptorHandleForHeapStart();
		CBVSRVUAV_Handle.ptr += (ImageIndex + 1) * CBVSRVUAV_DescriptorSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptor{};
		SRVDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDescriptor.Texture2D.MipLevels = 1;
		GUIWindow.DirectXContext->Device->CreateShaderResourceView(GUIImage.Resource, &SRVDescriptor, CBVSRVUAV_Handle);
	}
	else
	{
		D3D12_RESOURCE_DESC ResourceDesc = GUIImage.Resource->GetDesc();
		if (ResourceDesc.Width != GUIImage.Width || ResourceDesc.Height != GUIImage.Height)
		{

			GUIImage.Resource->Release();
			GUIImage.UploadResource->Unmap(0, nullptr);
			GUIImage.UploadResource->Release();

			// Texture Resource
			{
				D3D12_RESOURCE_DESC ResourceDesc{};
				ResourceDesc.DepthOrArraySize = 1;
				ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				ResourceDesc.Width = GUIImage.Width;
				ResourceDesc.Height = GUIImage.Height;
				ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				ResourceDesc.MipLevels = 1;
				ResourceDesc.SampleDesc.Count = 1;
				ResourceDesc.SampleDesc.Quality = 0;

				GUIWindow.DirectXContext->Device->CreateCommittedResource(
					&DefaultHeapProperties,
					D3D12_HEAP_FLAG_NONE,
					&ResourceDesc,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					nullptr,
					IID_PPV_ARGS(&GUIImage.Resource));
			}

			// Upload Resource
			{
				D3D12_RESOURCE_DESC UploadResourceDesc{};
				UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				UploadResourceDesc.Alignment = 0;
				UploadResourceDesc.Width = AlignedRowPitch * GUIImage.Height;
				UploadResourceDesc.Height = 1;
				UploadResourceDesc.DepthOrArraySize = 1;
				UploadResourceDesc.MipLevels = 1;
				UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
				UploadResourceDesc.SampleDesc.Count = 1;
				UploadResourceDesc.SampleDesc.Quality = 0;
				UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				UploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				Result = GUIWindow.DirectXContext->Device->CreateCommittedResource(
					&UploadHeapProperties,
					D3D12_HEAP_FLAG_NONE,
					&UploadResourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&GUIImage.UploadResource));

				D3D12_RANGE UploadRange{ 0, AlignedRowPitch * GUIImage.Height };
				GUIImage.UploadResource->Map(0, &UploadRange, &GUIImage.UploadData);
			}

			SIZE_T CBVSRVUAV_DescriptorSize = GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			D3D12_CPU_DESCRIPTOR_HANDLE CBVSRVUAV_Handle =
				GUIWindow.CBVSRVUAV_DescHeap->GetCPUDescriptorHandleForHeapStart();
			CBVSRVUAV_Handle.ptr += (ImageIndex + 1) * CBVSRVUAV_DescriptorSize;

			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptor{};
			SRVDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRVDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			SRVDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDescriptor.Texture2D.MipLevels = 1;
			GUIWindow.DirectXContext->Device->CreateShaderResourceView(
				GUIImage.Resource,
				&SRVDescriptor,
				CBVSRVUAV_Handle);
		}
	}

	for (uint64_t Row = 0; Row < GUIImage.Height; Row++)
	{
		memcpy(GUIImage.UploadBytes + Row * AlignedRowPitch, GUIImage.Bytes + Row * SourceRowPitch, SourceRowPitch);
	}

	// Upload
	GUIWindow.DirectXContext->CopyCommandList->Reset(GUIWindow.DirectXContext->CopyCommandAllocator, nullptr);
	CD3DX12_RESOURCE_BARRIER StartCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		GUIImage.Resource,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	GUIWindow.DirectXContext->CopyCommandList->ResourceBarrier(1, &StartCopyBarrier);

	// Get the copy target location
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT SourceFootprint = {};
	SourceFootprint.Footprint.Width = static_cast<UINT>(GUIImage.Width);
	SourceFootprint.Footprint.Height = static_cast<UINT>(GUIImage.Height);
	SourceFootprint.Footprint.Depth = 1;
	SourceFootprint.Footprint.RowPitch = AlignedRowPitch;
	SourceFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_TEXTURE_COPY_LOCATION CopyDest(GUIImage.Resource, 0);
	CD3DX12_TEXTURE_COPY_LOCATION CopySrc(GUIImage.UploadResource, SourceFootprint);

	GUIWindow.DirectXContext->CopyCommandList->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySrc, nullptr);

	CD3DX12_RESOURCE_BARRIER FinishCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		GUIImage.Resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	GUIWindow.DirectXContext->CopyCommandList->ResourceBarrier(1, &FinishCopyBarrier);
	GUIWindow.DirectXContext->CopyCommandList->Close();

	std::vector<ID3D12CommandList*> CommandLists{};
	CommandLists.push_back(GUIWindow.DirectXContext->CopyCommandList);
	GUIWindow.DirectXContext->CopyCommandQueue->ExecuteCommandLists(CommandLists.size(), CommandLists.data());

	UINT64 FenceValue = 0;
	if (!HDirectX::SignalFence(
			GUIWindow.DirectXContext->CopyCommandQueue,
			GUIWindow.DirectXContext->CopyFence,
			FenceValue))
	{
		return false;
	}

	HDirectX::WaitForFence(GUIWindow.DirectXContext->CopyFence, FenceValue);

	return true;
}

void HImGui::DrawImage(HGUIWindow& GUIWindow, int64_t ImageIndex)
{
	if (ImageIndex < 0 || ImageIndex >= GUIWindow.ImageCount)
	{
		return;
	}

	HGUIImage& GUIImage = GUIWindow.Images[ImageIndex];

	SIZE_T CBVSRVUAV_DescriptorSize =
		GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_GPU_DESCRIPTOR_HANDLE CBVSRVUAV_Handle = GUIWindow.CBVSRVUAV_DescHeap->GetGPUDescriptorHandleForHeapStart();
	CBVSRVUAV_Handle.ptr += (ImageIndex + 1) * CBVSRVUAV_DescriptorSize;

	ImGui::Image(
		reinterpret_cast<ImTextureID>(CBVSRVUAV_Handle.ptr),
		ImVec2{ static_cast<float>(GUIImage.Width), static_cast<float>(GUIImage.Height) });
}

bool HImGui::DestroyImage(HGUIWindow& GUIWindow, int64_t ImageIndex)
{
	HGUIImage& GUIImage = GUIWindow.Images[ImageIndex];

	if (GUIImage.Resource != nullptr)
	{
		GUIImage.Resource->Release();
		GUIImage.Resource = nullptr;
	}

	GUIWindow.ImageRecycling.push_back(ImageIndex);

	return true;
}