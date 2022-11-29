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

bool HDirectX::CreateRTVHeap(ID3D12DescriptorHeap** RTVDescHeap, ID3D12Device* Device, uint32_t DescriptorCount)
{
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = DescriptorCount;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1;
		if (Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(RTVDescHeap)) != S_OK)
		{
			return false;
		}

		return true;
	}
}

bool HDirectX::CreateCBVSRVUAVHeap(
	ID3D12DescriptorHeap** CBVSRVUAVDescHeap,
	ID3D12Device* Device,
	uint32_t DescriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = DescriptorCount;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(CBVSRVUAVDescHeap)) != S_OK)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateCommandQueue(ID3D12CommandQueue** CommandQueue, ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = Type;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 1;
	if (Device->CreateCommandQueue(&desc, IID_PPV_ARGS(CommandQueue)) != S_OK)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateCommandAllocator(
	ID3D12CommandAllocator** CommandAllocator,
	ID3D12Device* Device,
	D3D12_COMMAND_LIST_TYPE Type)
{
	if (Device->CreateCommandAllocator(Type, IID_PPV_ARGS(CommandAllocator)) != S_OK)
	{
		return false;
	}
	return true;
}

bool HDirectX::CreateCommandList(
	ID3D12GraphicsCommandList** CommandList,
	ID3D12CommandAllocator* CommandAllocator,
	ID3D12Device* Device,
	D3D12_COMMAND_LIST_TYPE Type)
{
	if (Device->CreateCommandList(0, Type, CommandAllocator, NULL, IID_PPV_ARGS(CommandList))
			!= S_OK
		|| (*CommandList)->Close() != S_OK)
	{
		return false;
	}
	return true;
}

bool HDirectX::CreateFence(HFence& Fence, ID3D12Device* Device)
{
	if (Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence.Fence)) != S_OK)
	{
		return false;
	}

	Fence.FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (Fence.FenceEvent == NULL)
	{
		return false;
	}

	return true;
}

bool HDirectX::CreateSwapChain(
	HSwapChain& SwapChain,
	HWND HWND,
	uint32_t BufferCount,
	ID3D12CommandQueue* CommandQueue,
	ID3D12Device* Device)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	{
		ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));
		SwapChainDesc.BufferCount = BufferCount;
		SwapChainDesc.Width = 0;
		SwapChainDesc.Height = 0;
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		SwapChainDesc.Stereo = FALSE;
	}

	{
		IDXGIFactory4* dxgiFactory = NULL;
		IDXGISwapChain1* swapChain1 = NULL;
		if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
			return false;
		if (dxgiFactory->CreateSwapChainForHwnd(CommandQueue, HWND, &SwapChainDesc, NULL, NULL, &swapChain1) != S_OK)
			return false;
		if (swapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain.SwapChain)) != S_OK)
			return false;
		swapChain1->Release();
		dxgiFactory->Release();
		SwapChain.SwapChain->SetMaximumFrameLatency(BufferCount);
		SwapChain.SwapChainWaitableObject = SwapChain.SwapChain->GetFrameLatencyWaitableObject();
	}

	return true;
}

bool HDirectX::SignalFence(ID3D12CommandQueue* CommandQueue, HFence& Fence, UINT64& FenceValue)
{
	FenceValue = Fence.LastSignaledValue + 1;
	HRESULT Result = CommandQueue->Signal(Fence.Fence, FenceValue);
	if (FAILED(Result))
	{
		FenceValue = 0;
		return false;
	}
	Fence.LastSignaledValue = FenceValue;

	return true;
}

void HDirectX::WaitForFence(HFence& Fence, UINT64& FenceValue)
{
	if (Fence.Fence->GetCompletedValue() >= FenceValue)
	{
		return;
	}

	Fence.Fence->SetEventOnCompletion(FenceValue, Fence.FenceEvent);
	WaitForSingleObject(Fence.FenceEvent, INFINITE);
}