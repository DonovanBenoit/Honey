#include "HDirectX.h"

bool HDirectX::CreateDeviceD3D(ID3D12Device** Device, HWND HWND)
{

	// [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
	ID3D12Debug* pdx12Debug = NULL;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
		pdx12Debug->EnableDebugLayer();
#endif

	// Create device
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(Device)) != S_OK)
	{
		return false;
	}

	// [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
	if (pdx12Debug != NULL)
	{
		ID3D12InfoQueue* pInfoQueue = NULL;
		(*Device)->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		pInfoQueue->Release();
		pdx12Debug->Release();
	}
#endif
	return true;
}

bool HDirectX::CreateRTVHeap(ID3D12DescriptorHeap** RTVDescHeap, ID3D12Device* Device)
{
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = HDirectXContext::NUM_BACK_BUFFERS;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1;
		if (Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(RTVDescHeap)) != S_OK)
		{
			return false;
		}

		return true;
	}
}

bool HDirectX::CreateCBVSRVUAVHeap(ID3D12DescriptorHeap** CBVSRVUAVDescHeap, ID3D12Device* Device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(CBVSRVUAVDescHeap)) != S_OK)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateCommandQueue(ID3D12CommandQueue** CommandQueue, ID3D12Device* Device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 1;
	if (Device->CreateCommandQueue(&desc, IID_PPV_ARGS(CommandQueue)) != S_OK)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateCommandAllocator(ID3D12CommandAllocator** CommandAllocator, ID3D12Device* Device)
{
	if (Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandAllocator)) != S_OK)
	{
		return false;
	}
	return true;
}

bool HDirectX::CreateCommandList(
	ID3D12GraphicsCommandList** CommandList,
	ID3D12CommandAllocator* CommandAllocator,
	ID3D12Device* Device)
{
	if (Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, NULL, IID_PPV_ARGS(CommandList))
			!= S_OK
		|| (*CommandList)->Close() != S_OK)
	{
		return false;
	}
	return true;
}

bool HDirectX::CreateFence(HFence& Fence, ID3D12Device* Device)
{
	if (Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence.g_fence)) != S_OK)
	{
		return false;
	}

	Fence.g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (Fence.g_fenceEvent == NULL)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateSwapChain(HSwapChain& SwapChain, HWND HWND, ID3D12CommandQueue* CommandQueue, ID3D12Device* Device)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = HDirectXContext::NUM_BACK_BUFFERS;
		sd.Width = 0;
		sd.Height = 0;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.Stereo = FALSE;
	}

	{
		IDXGIFactory4* dxgiFactory = NULL;
		IDXGISwapChain1* swapChain1 = NULL;
		if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
			return false;
		if (dxgiFactory->CreateSwapChainForHwnd(CommandQueue, HWND, &sd, NULL, NULL, &swapChain1)
			!= S_OK)
			return false;
		if (swapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain.SwapChain)) != S_OK)
			return false;
		swapChain1->Release();
		dxgiFactory->Release();
		SwapChain.SwapChain->SetMaximumFrameLatency(HDirectXContext::NUM_BACK_BUFFERS);
		SwapChain.SwapChainWaitableObject = SwapChain.SwapChain->GetFrameLatencyWaitableObject();
	}

	return true;
}