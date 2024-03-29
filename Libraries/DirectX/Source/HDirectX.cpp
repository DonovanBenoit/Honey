#include "HDirectX.h"

namespace
{
	size_t AlignedSize(size_t Size, size_t ALignment)
	{
		return ((Size + ALignment - 1) / ALignment) * ALignment;
	}
} // namespace

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
	HDescriptorHeap& CBVSRVUAVDescHeap,
	ID3D12Device* Device,
	uint32_t DescriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DescriptorHeapDesc.NumDescriptors = DescriptorCount;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (Device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&CBVSRVUAVDescHeap.DescriptorHeap)) != S_OK)
	{
		return false;
	}

	CBVSRVUAVDescHeap.HeapIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
	if (Device->CreateCommandList(0, Type, CommandAllocator, NULL, IID_PPV_ARGS(CommandList)) != S_OK
		|| (*CommandList)->Close() != S_OK)
	{
		return false;
	}
	return true;
}

bool HDirectX::CreateRootSignature(
	Microsoft::WRL::ComPtr<ID3D12RootSignature>& RootSiganature,
	CD3DX12_ROOT_SIGNATURE_DESC& RootSignatureDesc,
	ID3D12Device* Device)
{
	ID3DBlob* SignatureBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Result =
		D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (Result != S_OK)
	{
		if (SignatureBlob != nullptr)
		{
			SignatureBlob->Release();
		}
		if (ErrorBlob != nullptr)
		{
			ErrorBlob->Release();
		}
		return false;
	}

	Result = Device->CreateRootSignature(
		0,
		SignatureBlob->GetBufferPointer(),
		SignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&RootSiganature));

	if (SignatureBlob != nullptr)
	{
		SignatureBlob->Release();
	}
	if (ErrorBlob != nullptr)
	{
		ErrorBlob->Release();
	}

	return Result == S_OK;
}

#include <d3dcompiler.h>
// #include <wrl/client.h>

bool HDirectX::CreateComputePipelineState(
	Microsoft::WRL::ComPtr<ID3D12PipelineState>& PipelineState,
	const std::filesystem::path& ShaderPath,
	const std::string_view& EntryPoint,
	ID3D12RootSignature* RootSiganature,
	ID3D12Device* Device)
{
	// Compile the compute shader
	Microsoft::WRL::ComPtr<ID3DBlob> ComputeShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
	HRESULT Result = D3DCompileFromFile(
		ShaderPath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint.data(),
		"cs_5_0",
		0,
		0,
		&ComputeShaderBlob,
		&ErrorBlob);
	if (FAILED(Result))
	{
		// Handle compilation error
		if (ErrorBlob)
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
		}
		return false;
	}

	// Create the compute shader
	D3D12_SHADER_BYTECODE ComputeShaderBytecode = {};
	ComputeShaderBytecode.pShaderBytecode = ComputeShaderBlob->GetBufferPointer();
	ComputeShaderBytecode.BytecodeLength = ComputeShaderBlob->GetBufferSize();

	D3D12_COMPUTE_PIPELINE_STATE_DESC Desc{};
	Desc.pRootSignature = RootSiganature;
	Desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Desc.CS = ComputeShaderBytecode;

	Result = Device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&PipelineState));

	return Result == S_OK;
}

bool HDirectX::CreateOrUpdateUnorderedTextureResource(
	HResource& Resource,
	ID3D12Device* Device,
	const glm::uvec2& Resolution,
	DXGI_FORMAT Format)
{
	if (Resource.Resource != nullptr)
	{
		D3D12_RESOURCE_DESC TextureDesc = (Resource.Resource)->GetDesc();
		if (TextureDesc.Width == Resolution.x && TextureDesc.Height == Resolution.y)
		{
			return true;
		}

		ULONG Count = (Resource.Resource)->Release();
		(Resource.Resource) = nullptr;
	}

	assert(Resolution.x > 0);
	assert(Resolution.y > 0);

	D3D12_RESOURCE_DESC TextureDesc = {};
	TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	TextureDesc.Alignment = 0;
	TextureDesc.Width = Resolution.x;
	TextureDesc.Height = Resolution.y;
	TextureDesc.DepthOrArraySize = 1;
	TextureDesc.MipLevels = 1;
	TextureDesc.Format = Format;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	TextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// Create the texture resource
	CD3DX12_HEAP_PROPERTIES DefaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT Result = Device->CreateCommittedResource(
		&DefaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&TextureDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Resource.Resource));

	return SUCCEEDED(Result);
}

bool HDirectX::CreateOrUpdateUnorderedBufferResource(
	ID3D12Resource** Resource,
	ID3D12Device* Device,
	size_t ElementSize,
	size_t ElementCount)
{
	if (*Resource != nullptr)
	{
		D3D12_RESOURCE_DESC BufferDesc = (*Resource)->GetDesc();
		if (BufferDesc.Width == AlignedSize(ElementSize, 4) * ElementCount)
		{
			return true;
		}

		ULONG Count = (*Resource)->Release();
		(*Resource) = nullptr;
	}

	CD3DX12_HEAP_PROPERTIES DefaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
		AlignedSize(ElementSize, 4) * ElementCount,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	HRESULT Result = Device->CreateCommittedResource(
		&DefaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(Resource));
	return SUCCEEDED(Result);
}

void HDirectX::CopyDataToResource(
	ID3D12Resource* Resource,
	ID3D12Resource* UploadResource,
	ID3D12Device* Device,
	ID3D12GraphicsCommandList* CommandList,
	void* Data,
	size_t Size)
{
	assert(Size > 0);

	bool CreateUploadBuffer = UploadResource == nullptr;
	if (UploadResource != nullptr)
	{
		D3D12_RESOURCE_DESC UploadDesc = UploadResource->GetDesc();
		if (UploadDesc.Width != Size)
		{
			CreateUploadBuffer = true;
			UploadResource->Release();
			UploadResource = nullptr;
		}
	}

	if (CreateUploadBuffer)
	{
		D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		HRESULT Result = Device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&UploadResource));
		if (FAILED(Result))
		{
			return;
		}
	}

	// Copy the data to the upload heap
	void* MappedData;
	UploadResource->Map(0, nullptr, &MappedData);
	memcpy(MappedData, Data, Size);
	UploadResource->Unmap(0, nullptr);

	// Transition the resource to the appropriate state for Copy
	D3D12_RESOURCE_BARRIER StartBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		Resource,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->ResourceBarrier(1, &StartBarrier);

	// Copy the data to the resource
	CommandList->CopyBufferRegion(Resource, 0, UploadResource, 0, Size);

	// Transition the resource to the appropriate state for use
	D3D12_RESOURCE_BARRIER EndBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		Resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	CommandList->ResourceBarrier(1, &EndBarrier);
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
		IDXGISwapChain1* SwapChain1 = NULL;
		if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
			return false;
		if (dxgiFactory->CreateSwapChainForHwnd(CommandQueue, HWND, &SwapChainDesc, NULL, NULL, &SwapChain1) != S_OK)
			return false;
		if (SwapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain.SwapChain)) != S_OK)
			return false;
		SwapChain1->Release();
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