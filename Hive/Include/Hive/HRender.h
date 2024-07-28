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

struct HComputePass
{
	ID3D12CommandQueue* CommandQueue = nullptr;
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	ID3D12GraphicsCommandList* CommandList = nullptr;
	ID3D12GraphicsCommandList* UpdateCommandList = nullptr;
	ID3D12CommandAllocator* UpdateCommandAllocator = nullptr;
	HFence Fence{};
	uint64_t FenceValue = 0;

	std::atomic<bool> TriggerShaderRebuild = false;

	HDescriptorHeap CBVSRVUAVDescriptorHeap{};

	HRootSignature RootSignature{};

	// Microsoft::WRL::ComPtr<ID3D12PipelineState> GetSphereDistancePipelineState = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState = nullptr;

	glm::vec2 Resolution{};

	HResource OutputResource{};
	HDescriptor OutputDescriptor{};

	/*ID3D12Resource* MarchDistanceResource = nullptr;
	int64_t MarchDistanceHeapIndex = -1;

	ID3D12Resource* StepDistanceResource = nullptr;
	int64_t StepDistanceHeapIndex = -1;*/

	ID3D12Resource* SpheresResource = nullptr;
	ID3D12Resource* SpheresUploadResource = nullptr;
	HDescriptor SpheresDescriptor{};

	ID3D12Resource* MaterialsResource = nullptr;
	ID3D12Resource* MaterialsUploadResource = nullptr;
	HDescriptor MaterialsDescriptor{};

	ID3D12Resource* SceneResource = nullptr;
	ID3D12Resource* SceneUploadResource = nullptr;
	HDescriptor SceneDescriptor{};

	ID3D12Resource* SDFsResource = nullptr;
	ID3D12Resource* SDFsUploadResource = nullptr;
	HDescriptor SDFsDescriptor{};

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