#pragma once

#ifdef _WIN32
#include <HDirectX.h>
#endif // _WIN32

#include <string>

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
	uint32_t ShaderSpace = 0;
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
		uint32_t NumDescriptors = 1,
		uint32_t ShaderSpace = 0,
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