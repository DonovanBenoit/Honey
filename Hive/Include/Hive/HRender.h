#pragma once

#include <atomic>
#include <entt/entt.hpp>
#include <future>
#include <glm/glm.hpp>

#ifdef _WIN32
#include <HDirectX.h>
#endif // _WIN32

struct HGUIImage;
struct HGUIWindow;
struct HScene;

enum class HRootParameterType : uint32_t
{
	Unknown,
	UAV
};

struct HRootParameter
{
	std::string Name = "";
	HRootParameterType RootParameterType = HRootParameterType::Unknown;
	uint32_t ShaderRegister = 0;
	uint32_t DescriptorRangeOffset = 0;
};

struct HRootSignature
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE> DescriptorRanges{};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSiganature = nullptr;

	std::vector<HRootParameter> RootParameters{};

	void AddRootParameter(std::string_view Name, HRootParameterType RootParameterType);

	bool Build(HGUIWindow& GUIWindow);

private:
	uint32_t UAVRegisterCount = 0;
};

template<uint64_t Size>
struct HCBVSRVUAVDescriptorHeap
{
	bool Create(ID3D12Device* Device)
	{
		DescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (!HDirectX::CreateCBVSRVUAVHeap(&DescriptorHeap, Device, Size))
		{
			return false;
		}

		CPUHandleForHeapStart = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		GPUHandleForHeapStart = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		return true;
	}

	bool CreateOrUpdateHandle(int64_t& HeapIndex, ID3D12Resource* Resource, uint64_t Count, ID3D12Device* Device)
	{
		if (HeapIndex < 0)
		{
			if (Offset >= Size)
			{
				return false;
			}
			HeapIndex = Offset;
			Offset++;
		}

		if (HeapIndex >= Size)
		{
			return false;
		}

		// UAV
		D3D12_RESOURCE_DESC ResourceDesc = Resource->GetDesc();
		if (ResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
			switch (ResourceDesc.Dimension)
			{
				case D3D12_RESOURCE_DIMENSION_BUFFER:
				{
					UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					UAVDesc.Format = ResourceDesc.Format;
					UAVDesc.Buffer.NumElements = Count;
					assert(ResourceDesc.Width % Count == 0);
					UAVDesc.Buffer.StructureByteStride = ResourceDesc.Width / Count;
					UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
					break;
				}
				case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				{
					UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					UAVDesc.Format = ResourceDesc.Format;
					UAVDesc.Texture2D.MipSlice = 0;
					UAVDesc.Texture2D.PlaneSlice = 0;
					break;
				}
				default:
					return false;
			}

			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle = CPUHandleForHeapStart;
			UAVHandle.Offset(DescriptorSize * HeapIndex);
			Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, UAVHandle);
			return true;
		}

		return false;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int64_t HeapIndex)
	{
		if (HeapIndex < 0 || HeapIndex >= Size)
		{
			return {};
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle = GPUHandleForHeapStart;
		GPUHandle.Offset(DescriptorSize * HeapIndex);
		return GPUHandle;
	}

	ID3D12DescriptorHeap* DescriptorHeap = nullptr;
	int64_t Offset = 0;
	uint32_t DescriptorSize = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandleForHeapStart{};
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandleForHeapStart{};
};

struct HComputePass
{
	ID3D12CommandQueue* CommandQueue = nullptr;
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	ID3D12GraphicsCommandList* CommandList = nullptr;
	HFence Fence{};
	uint64_t FenceValue = 0;

	std::atomic<bool> TriggerShaderRebuild = false;

	HCBVSRVUAVDescriptorHeap<16384> CBVSRVUAVDescriptorHeap;

	HRootSignature RootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState = nullptr;

	glm::vec2 Resolution{};

	ID3D12Resource* OutputResource = nullptr;
	int64_t OutputHeapIndex = -1;

	ID3D12Resource* SpheresResource = nullptr;
	int64_t SpheresHeapIndex = -1;

	ID3D12Resource* MaterialsResource = nullptr;
	int64_t MaterialsHeapIndex = -1;

	ID3D12Resource* SceneResource = nullptr;
	int64_t SceneHeapIndex = -1;

	ID3D12Resource* SDFsResource = nullptr;
	int64_t SDFsHeapIndex = -1;

	std::future<bool> RenderFuture{};
};

struct HRenderWindow
{
	HComputePass ComputePass{};
	int64_t ImageIndex = -1;
};

namespace HHoney
{
	void DrawRender(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow);

	bool CreatComputePass(HGUIWindow& GUIWindow, HComputePass& ComputePass);
	bool RenderComputePass(
		HGUIWindow& GUIWindow,
		HComputePass& ComputePass,
		HScene& Scene,
		entt::entity CameraEntity,
		const glm::vec2& Resolution);

	void ControlsWindow(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow);
} // namespace HHoney