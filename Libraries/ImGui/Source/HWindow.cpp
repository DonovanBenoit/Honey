#include "HWindow.h"

#include "HImGui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_glfw.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include <directx/d3dx12.h>
#include <tchar.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of helper functions
void CleanupDeviceD3D();

namespace
{
	HDirectXContext DirectXContext{};

	ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
} // namespace

bool HImGui::CreateGUIWindow(HGUIWindow& GUIWindow)
{
	// Create GLFW Window
	glfwInit();
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	GLFWmonitor* PrimaryMonitor = glfwGetPrimaryMonitor();
	int32_t XPos;
	int32_t YPos;
	int32_t Width;
	int32_t Height;
	glfwGetMonitorWorkarea(PrimaryMonitor, &XPos, &YPos, &Width, &Height);
	glfwWindowHint(GLFW_POSITION_X, XPos);
	glfwWindowHint(GLFW_POSITION_Y, XPos);
	GUIWindow.Window = glfwCreateWindow(Width, Height, "Hive", nullptr, nullptr);

	// Get Windows Handle
	GUIWindow.WindowHandle = glfwGetWin32Window(GUIWindow.Window);

	// Shared DirectX Context
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
	SIZE_T RTVDescriptorSize =
		GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = GUIWindow.RTV_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < HGUIWindow::NUM_BACK_BUFFERS; i++)
	{
		GUIWindow.RenderTargetDescriptor[i] = RTVHandle;
		RTVHandle.ptr += RTVDescriptorSize;
	}

	// CBVSRVUAV
	if (!HDirectX::CreateCBVSRVUAVHeap(GUIWindow.CBVSRVUAV_DescHeap, GUIWindow.DirectXContext->Device, 1000000))
	{
		HImGui::DestroyGUIWindow(GUIWindow);
		return false;
	}

	GUIWindow.ImGuiDescriptor = GUIWindow.CBVSRVUAV_DescHeap.AllocateDescriptor();

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

		// Fence, each thread needs their own fence
		if (!HDirectX::CreateFence(DirectXContext.FrameContext[i].Fence, GUIWindow.DirectXContext->Device))
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

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImFont* Font = ImGui::GetIO().Fonts->AddFontFromFileTTF("C://Windows//Fonts//Consola.ttf", 14);

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(GUIWindow.Window, true);
	ImGui_ImplDX12_Init(
		GUIWindow.DirectXContext->Device,
		HDirectXContext::NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		GUIWindow.CBVSRVUAV_DescHeap.DescriptorHeap,
		GUIWindow.ImGuiDescriptor.CPUDescriptorHandle,
		GUIWindow.ImGuiDescriptor.GPUDescriptorHandle);

	return true;
}

void HImGui::NewFrame(HGUIWindow& GUIWindow, bool& Quit)
{
	Quit = Quit || glfwWindowShouldClose(GUIWindow.Window);
	if (Quit)
	{
		return;
	}

	// Resize Windows if Necessary
	int32_t WindowWidth;
	int32_t WindowHeight;
	glfwGetWindowSize(GUIWindow.Window, &WindowWidth, &WindowHeight);
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	HRESULT Result = GUIWindow.SwapChain.SwapChain->GetDesc1(&SwapChainDesc);
	if (FAILED(Result))
	{
		Quit = true;
		return;
	}
	if (SwapChainDesc.Width != WindowWidth || SwapChainDesc.Height != WindowHeight)
	{
		// Wait for previous frames to finish rendering
		WaitForAllSubmittedFrames(GUIWindow);

		HImGui::DestroyRenderTargets(GUIWindow);

		HRESULT Result = GUIWindow.SwapChain.SwapChain->ResizeBuffers(
			0,
			WindowWidth,
			WindowHeight,
			DXGI_FORMAT_UNKNOWN,
			DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
		assert(SUCCEEDED(Result) && "Failed to resize swapchain.");
		HImGui::CreateRenderTargets(GUIWindow);
	}

	glfwPollEvents();

	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

bool HImGui::Render(HGUIWindow& GUIWindow)
{
	// Rendering
	ImGui::Render();

	HFrameContext* FrameContext = HImGui::WaitForNextFrameResources(GUIWindow);
	UINT BackBufferIndex = GUIWindow.SwapChain.SwapChain->GetCurrentBackBufferIndex();

	// Reset Command Allocator
	FrameContext->CommandAllocator->Reset();
	GUIWindow.DirectXContext->CommandList->Reset(FrameContext->CommandAllocator, NULL);

	// Render Target Transition Barrier
	D3D12_RESOURCE_BARRIER RenderTargetBarrier = {};
	RenderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	RenderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	RenderTargetBarrier.Transition.pResource = GUIWindow.RenderTargetResource[BackBufferIndex];
	RenderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// Transition To Render Target
	RenderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	RenderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	GUIWindow.DirectXContext->CommandList->ResourceBarrier(1, &RenderTargetBarrier);

	// Render Dear ImGui graphics
	GUIWindow.DirectXContext->CommandList->ClearRenderTargetView(
		GUIWindow.RenderTargetDescriptor[BackBufferIndex],
		reinterpret_cast<float*>(&ClearColor),
		0,
		NULL);
	GUIWindow.DirectXContext->CommandList
		->OMSetRenderTargets(1, &GUIWindow.RenderTargetDescriptor[BackBufferIndex], FALSE, NULL);
	GUIWindow.DirectXContext->CommandList->SetDescriptorHeaps(1, &GUIWindow.CBVSRVUAV_DescHeap.DescriptorHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUIWindow.DirectXContext->CommandList);

	// Transition To Present
	RenderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	RenderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	GUIWindow.DirectXContext->CommandList->ResourceBarrier(1, &RenderTargetBarrier);

	// Close and execute command lists
	GUIWindow.DirectXContext->CommandList->Close();
	GUIWindow.DirectXContext->CommandQueue->ExecuteCommandLists(
		1,
		(ID3D12CommandList* const*)&GUIWindow.DirectXContext->CommandList);

	// Wait for the commands to finish executing
	if (!HDirectX::SignalFence(GUIWindow.DirectXContext->CommandQueue, FrameContext->Fence, FrameContext->FenceValue))
	{
		return false;
	}

	// Present with vsync
	GUIWindow.SwapChain.SwapChain->Present(1, 0);

	return true;
}

void HImGui::DestroyGUIWindow(HGUIWindow& GUIWindow)
{
	WaitForAllSubmittedFrames(GUIWindow);

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
	GUIWindow.CBVSRVUAV_DescHeap.Release();

	CleanupDeviceD3D();

	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplDX12_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(GUIWindow.Window);
}

HFrameContext* HImGui::WaitForNextFrameResources(HGUIWindow& GUIWindow)
{
	UINT NextFrameIndex = DirectXContext.FrameIndex + 1;
	DirectXContext.FrameIndex = NextFrameIndex;

	HANDLE WaitableObjects[] = { GUIWindow.SwapChain.SwapChainWaitableObject, NULL };
	DWORD NumWaitableObjects = 1;

	HFrameContext* FrameContext = &DirectXContext.FrameContext[NextFrameIndex % HDirectXContext::NUM_FRAMES_IN_FLIGHT];
	UINT64 FenceValue = FrameContext->FenceValue;
	// if FenceValue == 0, no fence was signaled (i.e. First frame)
	if (FenceValue != 0)
	{
		FrameContext->Fence.Fence->SetEventOnCompletion(FenceValue, FrameContext->Fence.FenceEvent);
		WaitableObjects[1] = FrameContext->Fence.FenceEvent;
		NumWaitableObjects = 2;
	}

	WaitForMultipleObjects(NumWaitableObjects, WaitableObjects, TRUE, INFINITE);

	return FrameContext;
}

void HImGui::WaitForAllSubmittedFrames(HGUIWindow& GUIWindow)
{
	uint64_t WaitiableObjectIndex = 0;
	std::array<HANDLE, HDirectXContext::NUM_FRAMES_IN_FLIGHT + 1> WaitableObjects;
	WaitableObjects[WaitiableObjectIndex++] = GUIWindow.SwapChain.SwapChainWaitableObject;

	for (int32_t FrameIndex = 0; FrameIndex < HDirectXContext::NUM_FRAMES_IN_FLIGHT; FrameIndex++)
	{
		HFrameContext& FrameContext = DirectXContext.FrameContext[FrameIndex];

		UINT64 FenceValue = FrameContext.FenceValue;
		if (FenceValue != 0)
		{
			FrameContext.Fence.Fence->SetEventOnCompletion(FenceValue, FrameContext.Fence.FenceEvent);
			WaitableObjects[WaitiableObjectIndex++] = FrameContext.Fence.FenceEvent;
		}
	}
	WaitForMultipleObjects(WaitableObjects.size(), WaitableObjects.data(), TRUE, INFINITE);
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
			DirectXContext.FrameContext[i].Fence.Release();
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

	if (DirectXContext.Device)
	{
		DirectXContext.Device->Release();
		DirectXContext.Device = NULL;
	}

#ifdef DX12_ENABLE_DEBUG_LAYER
	IDXGIDebug1* Debug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&Debug))))
	{
		Debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		Debug->Release();
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

glm::vec2 HImGui::GetWindowSize(HGUIWindow& GUIWindow)
{
	int32_t WindowWidth;
	int32_t WindowHeight;
	glfwGetWindowSize(GUIWindow.Window, &WindowWidth, &WindowHeight);
	return glm::vec2(WindowWidth, WindowHeight);
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
	if (GUIImage.Descriptor.CPUDescriptorHandle.ptr == 0)
	{
		GUIImage.Descriptor = GUIWindow.CBVSRVUAV_DescHeap.AllocateDescriptor();
	}

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

namespace
{
	uint64_t CalculateAlignedSize(uint64_t Size, uint64_t Alignment)
	{
		return Size + (Alignment - (Size % Alignment)) % Alignment;
	}
} // namespace

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

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptor{};
		SRVDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDescriptor.Texture2D.MipLevels = 1;
		GUIWindow.DirectXContext->Device->CreateShaderResourceView(
			GUIImage.Resource,
			&SRVDescriptor,
			GUIImage.Descriptor.CPUDescriptorHandle);
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

			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptor{};
			SRVDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRVDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			SRVDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDescriptor.Texture2D.MipLevels = 1;
			GUIWindow.DirectXContext->Device->CreateShaderResourceView(
				GUIImage.Resource,
				&SRVDescriptor,
				GUIImage.Descriptor.CPUDescriptorHandle);
		}
	}

	for (uint64_t Row = 0; Row < GUIImage.Height; Row++)
	{
		memcpy(GUIImage.UploadBytes + Row * AlignedRowPitch, GUIImage.Bytes + Row * SourceRowPitch, SourceRowPitch);
	}

	// Upload
	GUIWindow.DirectXContext->CopyCommandAllocator->Reset();
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

	ImGui::Image(
		reinterpret_cast<ImTextureID>(GUIImage.Descriptor.GPUDescriptorHandle.ptr),
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