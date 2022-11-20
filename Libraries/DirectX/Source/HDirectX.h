#pragma once

#include <cinttypes>

#ifdef _WIN32

#include <d3d12.h>
#include <dxgi1_4.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

struct HFrameContext
{
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	UINT64 FenceValue = 0;
};

struct HFence
{
	ID3D12Fence* Fence = nullptr;
	HANDLE FenceEvent = nullptr;

	void Release()
	{
		if (Fence != nullptr)
		{
			Fence->Release();
			Fence = nullptr;
		}
		if (FenceEvent != nullptr)
		{
			CloseHandle(FenceEvent);
			FenceEvent = nullptr;
		}
	}
};

struct HSwapChain
{
	IDXGISwapChain3* SwapChain = nullptr;
	HANDLE SwapChainWaitableObject = nullptr;
};

struct HDirectXContext
{
	// Data
	static int32_t const NUM_FRAMES_IN_FLIGHT = 3;
	HFrameContext FrameContext[NUM_FRAMES_IN_FLIGHT] = {};
	UINT FrameIndex = 0;

	static int32_t const NUM_BACK_BUFFERS = 3;
	ID3D12Device* Device = nullptr;

	ID3D12DescriptorHeap* RTV_DescHeap = nullptr;
	ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
	
	ID3D12CommandQueue* CommandQueue = nullptr;
	ID3D12GraphicsCommandList* CommandList = nullptr;

	UINT64 g_fenceLastSignaledValue = 0;
	ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
	HSwapChain SwapChain{};
	HFence Fence{};

	static const uint64_t MaxImageCount = 1024;
	ID3D12Resource* ImageResources[MaxImageCount] = {};
};

namespace HDirectX
{
	bool CreateDeviceD3D(ID3D12Device** Device, HWND HWND);
	bool CreateRTVHeap(ID3D12DescriptorHeap** RTVDescHeap, ID3D12Device* Device);
	bool CreateCBVSRVUAVHeap(ID3D12DescriptorHeap** CBVSRVUAVDescHeap, ID3D12Device* Device);
	bool CreateCommandQueue(ID3D12CommandQueue** CommandQueue, ID3D12Device* Device);
	bool CreateCommandAllocator(ID3D12CommandAllocator** CommandAllocator, ID3D12Device* Device);
	bool CreateCommandList(ID3D12GraphicsCommandList** CommandList, ID3D12CommandAllocator* CommandAllocator, ID3D12Device* Device);
	bool CreateFence(HFence& Fence, ID3D12Device* Device);
	bool CreateSwapChain(HSwapChain& SwapChain, HWND HWND, ID3D12CommandQueue* CommandQueue, ID3D12Device* Device);
};

#endif // _WIN32