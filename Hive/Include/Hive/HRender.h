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
struct HInstancedMesh;
struct HMesh;
struct HScene;
struct HTexture;
struct HVertex;

enum class HRootParameterType : uint32_t
{
	Unknown,
	SRV,
	UAV,
	CBV,
};

enum class HShaderVisibility
{
	All,
	Vertex,
	Pixel,
};

struct HRootParameter
{
	std::string Name = "";
	HRootParameterType RootParameterType = HRootParameterType::Unknown;
	HShaderVisibility ShaderVisibility = HShaderVisibility::All;
	uint32_t ShaderRegister = 0;
	uint32_t DescriptorRangeOffset = 0;
};

enum class HRootParameterIndex : uint32_t
{
	Null = std::numeric_limits<uint32_t>::max()
};

struct HRootSignature
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE1> DescriptorRanges{};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSiganature = nullptr;

	std::vector<HRootParameter> RootParameters{};

	inline HRootParameter& operator[] (HRootParameterIndex Index)
	{
		return RootParameters[static_cast<uint32_t>(Index)];
	}

	HRootParameterIndex AddRootParameter(
		std::string_view Name,
		HRootParameterType RootParameterType,
		HShaderVisibility ShaderVisibility = HShaderVisibility::All);

	bool Build(
		HDirectXContext& DirectXContext,
		D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	void Release()
	{
		DescriptorRanges.clear();
		RootParameters.clear();

		if (RootSiganature)
		{
			RootSiganature.Reset();
		}
	}

private:
	uint32_t SRVRegisterCount = 0;
	uint32_t UAVRegisterCount = 0;
	uint32_t CBVRegisterCount = 0;
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

struct HSceneBuffer
{
	glm::vec4 Translation;
	glm::vec4 Scale;
};

struct HInstanceBuffer
{
	glm::vec4 Translation;
	glm::vec4 Padding[15];
};

struct HRenderPass
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
	HDescriptorHeap RTVDescriptorHeap{};

	std::vector<HDescriptor> TextureDescriptors{};

	HRootSignature RootSignature{};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState = nullptr;

	// Output
	uint64_t FrontBufferIndex = 0;
	static const uint64_t OutputBufferCount = 2;
	std::array<HResource, OutputBufferCount> OutputResources{};
	std::array<HDescriptor, OutputBufferCount> OutputRTVDescriptors{};

	// Scene Buffer
	HRootParameterIndex SceneBufferIndex = HRootParameterIndex::Null;
	std::array<HResource, OutputBufferCount> SceneBufferResources{};
	std::array<HDescriptor, OutputBufferCount> SceneBufferDescriptors{};
	std::array<HSceneBuffer*, OutputBufferCount> MappedSceneBuffers{};

	// Instance Buffer
	HRootParameterIndex InstanceBufferIndex = HRootParameterIndex::Null;
	std::array<HResource, OutputBufferCount> InstanceBufferResources{};
	std::array<HDescriptor, OutputBufferCount> InstanceBufferDescriptors{};
	std::array<HInstanceBuffer*, OutputBufferCount> MappedInstanceBuffers{};

	// Vertex
	HRootParameterIndex VertexBufferIndex = HRootParameterIndex::Null;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, OutputBufferCount> VertexBufferResources{};
	std::array<D3D12_VERTEX_BUFFER_VIEW, OutputBufferCount> VertexBufferViews{};
	std::array<HVertex*, OutputBufferCount> MappedVertexBufferData{};
};

struct HRenderWindow
{
	HComputePass ComputePass{};
	int64_t ImageIndex = -1;
};

namespace HHoney
{
	bool CreatRenderPass(HDirectXContext& DirectXContext, HRenderPass& RenderPass, const glm::vec2& Resolution);
	bool RenderRenderPass(
		HDirectXContext& DirectXContext,
		HRenderPass& RenderPass,
		const glm::vec3& Translation,
		const glm::vec3& Scale,
		const std::vector<HInstancedMesh>& InstanedMeshes,
		const HTexture& Texture,
		const glm::vec2& Resolution);
	void DestroyRenderPass(HRenderPass& RenderPass);
} // namespace HHoney

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