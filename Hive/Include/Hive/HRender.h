#pragma once

#include "HRootSignature.h"

#include <atomic>
#include <future>
#include <unordered_map>

#include <entt/entt.hpp>
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

struct HSceneBuffer
{
	glm::vec4 Translation;
	glm::vec4 Scale;
};

struct HInstanceBuffer
{
	glm::vec4 Translation;
};

struct HRenderPass
{
	ID3D12CommandQueue* CommandQueue = nullptr;
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	ID3D12GraphicsCommandList* CommandList = nullptr;
	std::shared_ptr<HFence> Fence = nullptr;
	uint64_t FenceValue = 0;

	std::atomic<bool> TriggerShaderRebuild = false;

	HDescriptorHeap CBVSRVUAVDescriptorHeap{};
	HDescriptorHeap RTVDescriptorHeap{};

	// Texture Array
	uint64_t TextureCount = 0;
	HRootParameterIndex TextureArrayRootParameter = HRootParameterIndex::Null;
	inline static const uint64_t MaxTextureCount = 512;
	std::array<HDescriptor, MaxTextureCount> TextureDescriptors{};
	std::unordered_map<entt::entity, uint64_t> TextureIndexMap{};

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
	std::array<std::vector<HDescriptor>, OutputBufferCount> InstanceBufferDescriptors{};
	std::array<HInstanceBuffer*, OutputBufferCount> MappedInstanceBuffers{};

	// Vertex
	HRootParameterIndex VertexBufferIndex = HRootParameterIndex::Null;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, OutputBufferCount> VertexBufferResources{};
	std::array<D3D12_VERTEX_BUFFER_VIEW, OutputBufferCount> VertexBufferViews{};
	std::array<HVertex*, OutputBufferCount> MappedVertexBufferData{};
};

namespace HHoney
{
	bool CreatRenderPass(HDirectXContext& DirectXContext, HRenderPass& RenderPass, const glm::vec2& Resolution);
	bool RenderRenderPass(
		HDirectXContext& DirectXContext,
		HRenderPass& RenderPass,
		const glm::vec3& Translation,
		const glm::vec3& Scale,
		const HScene& Scene,
		const std::vector<HInstancedMesh>& InstanedMeshes,
		const glm::vec2& Resolution);
	void DestroyRenderPass(HRenderPass& RenderPass);
} // namespace HHoney