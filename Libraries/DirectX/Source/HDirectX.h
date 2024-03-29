#pragma once

#include <array>
#include <cinttypes>
#include <filesystem>
#include <glm/glm.hpp>

#ifdef _WIN32

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

struct HFence
{
	ID3D12Fence* Fence = nullptr;
	HANDLE FenceEvent = nullptr;
	UINT64 LastSignaledValue = 0;

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

struct HFrameContext
{
	ID3D12CommandAllocator* CommandAllocator = nullptr;

	HFence Fence{};
	UINT64 FenceValue = 0;
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

	ID3D12Device* Device = nullptr;

	ID3D12CommandQueue* CommandQueue = nullptr;
	ID3D12GraphicsCommandList* CommandList = nullptr;

	ID3D12CommandQueue* CopyCommandQueue = nullptr;
	ID3D12GraphicsCommandList* CopyCommandList = nullptr;
	ID3D12CommandAllocator* CopyCommandAllocator = nullptr;
	HFence CopyFence{};
};

struct HResource
{
	uint64_t Version = 0;
	ID3D12Resource* Resource = nullptr;
};

struct HDescriptor
{
	uint64_t Index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = {};
};

struct HDescriptorHeap
{
	uint64_t Count = 0;
	ID3D12DescriptorHeap* DescriptorHeap = nullptr;
	uint64_t HeapIncrementSize = 0;

	void Release()
	{
		Count = 0;
		HeapIncrementSize = 0;
		if (DescriptorHeap != nullptr)
		{
			DescriptorHeap->Release();
			DescriptorHeap = nullptr;
		}
	}

	inline HDescriptor AllocateDescriptor()
	{
		HDescriptor Descriptor{};
		Descriptor.Index = Count;
		Descriptor.CPUDescriptorHandle = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Descriptor.CPUDescriptorHandle.ptr += HeapIncrementSize * Descriptor.Index;
		Descriptor.GPUDescriptorHandle = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		Descriptor.GPUDescriptorHandle.ptr += HeapIncrementSize * Descriptor.Index;
		Count++;
		return Descriptor;
	}
};

namespace HDirectX
{
	bool CreateDeviceD3D(ID3D12Device** Device, HWND HWND);
	bool CreateRTVHeap(ID3D12DescriptorHeap** RTVDescHeap, ID3D12Device* Device, uint32_t DescriptorCount);
	bool CreateCBVSRVUAVHeap(HDescriptorHeap& CBVSRVUAVDescHeap, ID3D12Device* Device, uint32_t DescriptorCount);
	bool CreateCommandQueue(ID3D12CommandQueue** CommandQueue, ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type);
	bool CreateCommandAllocator(
		ID3D12CommandAllocator** CommandAllocator,
		ID3D12Device* Device,
		D3D12_COMMAND_LIST_TYPE Type);
	bool CreateCommandList(
		ID3D12GraphicsCommandList** CommandList,
		ID3D12CommandAllocator* CommandAllocator,
		ID3D12Device* Device,
		D3D12_COMMAND_LIST_TYPE Type);

	bool CreateRootSignature(
		Microsoft::WRL::ComPtr<ID3D12RootSignature>& RootSiganature,
		CD3DX12_ROOT_SIGNATURE_DESC& RootSignatureDesc,
		ID3D12Device* Device);
	bool CreateComputePipelineState(
		Microsoft::WRL::ComPtr<ID3D12PipelineState>& PipelineState,
		const std::filesystem::path& ShaderPath,
		const std::string_view& EntryPoint,
		ID3D12RootSignature* RootSiganature,
		ID3D12Device* Device);

	bool CreateOrUpdateUnorderedTextureResource(
		HResource& Resource,
		ID3D12Device* Device,
		const glm::uvec2& Resolution,
		DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM);
	bool CreateOrUpdateUnorderedBufferResource(
		ID3D12Resource** Resource,
		ID3D12Device* Device,
		size_t ElementSize,
		size_t ElementCount);

	void CopyDataToResource(
		ID3D12Resource* Resource,
		ID3D12Resource* UploadResource,
		ID3D12Device* Device,
		ID3D12GraphicsCommandList* CommandList,
		void* Data,
		size_t Size);

	bool CreateFence(HFence& Fence, ID3D12Device* Device);
	bool CreateSwapChain(
		HSwapChain& SwapChain,
		HWND HWND,
		uint32_t BufferCount,
		ID3D12CommandQueue* CommandQueue,
		ID3D12Device* Device);
	bool SignalFence(ID3D12CommandQueue* CommandQueue, HFence& Fence, UINT64& FenceValue);
	void WaitForFence(HFence& Fence, UINT64& FenceValue);

	template<size_t Count>
	void ExecuteCommandLists(ID3D12CommandQueue* CommandQueue, const std::array<ID3D12CommandList*, Count> CommandLists)
	{
		CommandQueue->ExecuteCommandLists(Count, CommandLists.data());
	}

}; // namespace HDirectX

#endif // _WIN32