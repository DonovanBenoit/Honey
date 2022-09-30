#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

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

struct HDirectXContext
{
	// Data
	static int const NUM_FRAMES_IN_FLIGHT = 3;
	HFrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
	UINT g_frameIndex = 0;

	static int const NUM_BACK_BUFFERS = 3;
	ID3D12Device* g_pd3dDevice = nullptr;
	ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
	ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
	ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
	ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
	ID3D12Fence* g_fence = nullptr;
	HANDLE g_fenceEvent = nullptr;
	UINT64 g_fenceLastSignaledValue = 0;
	IDXGISwapChain3* g_pSwapChain = nullptr;
	HANDLE g_hSwapChainWaitableObject = nullptr;
	ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
};