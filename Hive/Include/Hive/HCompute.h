#pragma once

#include "HRootSignature.h"

#include <atomic>
#include <entt/entt.hpp>
#include <future>
#include <glm/glm.hpp>

#ifdef _WIN32
#include <HDirectX.h>
#endif // _WIN32

struct HGUIImage;
struct HGUIWindow;
struct HInstancedMesh;
struct HMesh;
struct HScene;
struct HTexture;
struct HVertex;

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

	HResource SpheresResource{};
	HDescriptor SpheresDescriptor{};

	HResource MaterialsResource{};
	HDescriptor MaterialsDescriptor{};

	HResource SceneResource{};
	HDescriptor SceneDescriptor{};

	HResource SDFsResource{};
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
	bool CreatComputePass(HGUIWindow& GUIWindow, HComputePass& ComputePass);

	bool RenderComputePass(
		HGUIWindow& GUIWindow,
		HComputePass& ComputePass,
		HScene& Scene,
		entt::entity CameraEntity,
		const glm::vec2& Resolution);

} // namespace HHoney