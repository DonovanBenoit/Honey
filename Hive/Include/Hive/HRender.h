#pragma once

#include <entt/entt.hpp>
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

	ID3D12DescriptorHeap* CBVSRVUAVDescHeap = nullptr;

	HRootSignature RootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState = nullptr;

	ID3D12Resource* OutputResource = nullptr;
};

struct HRenderWindow
{
	HComputePass ComputePass{};
	int64_t ImageIndex = -1;
};

namespace HHoney
{
	void Render(HScene& Scene, HGUIImage& Image, entt::entity CameraEntity);

	void DrawRender(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow);

	bool CreatComputePass(HGUIWindow& GUIWindow, HComputePass& ComputePass);
} // namespace HHoney