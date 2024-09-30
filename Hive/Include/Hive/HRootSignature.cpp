#include "HRootSignature.h"

HRootParameterIndex HRootSignature::AddRootParameter(
	std::string_view Name,
	HRootParameterType RootParameterType,
	HShaderVisibility ShaderVisibility)
{
	HRootParameterIndex Index = static_cast<HRootParameterIndex>(RootParameters.size());
	HRootParameter& RootParameter = RootParameters.emplace_back();
	RootParameter.Name = Name;
	RootParameter.RootParameterType = RootParameterType;
	RootParameter.ShaderVisibility = ShaderVisibility;
	switch (RootParameter.RootParameterType)
	{
		case HRootParameterType::SRV:
		{
			RootParameter.ShaderRegister = SRVRegisterCount++;
			RootParameter.DescriptorRangeOffset = static_cast<uint32_t>(DescriptorRanges.size());
			CD3DX12_DESCRIPTOR_RANGE1& DescriptorRange = DescriptorRanges.emplace_back();
			DescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, RootParameter.ShaderRegister);
		}
		break;
		case HRootParameterType::UAV:
		{
			RootParameter.ShaderRegister = UAVRegisterCount++;
			RootParameter.DescriptorRangeOffset = static_cast<uint32_t>(DescriptorRanges.size());
			CD3DX12_DESCRIPTOR_RANGE1& DescriptorRange = DescriptorRanges.emplace_back();
			DescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, RootParameter.ShaderRegister);
		}
		break;
		case HRootParameterType::CBV:
		{
			RootParameter.ShaderRegister = CBVRegisterCount++;
			RootParameter.DescriptorRangeOffset = static_cast<uint32_t>(DescriptorRanges.size());
			CD3DX12_DESCRIPTOR_RANGE1& DescriptorRange = DescriptorRanges.emplace_back();
			DescriptorRange.Init(
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				1,
				RootParameter.ShaderRegister,
				0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		}
		break;
		case HRootParameterType::Unknown:
		default:
			assert(false);
			break;
	}

	return Index;
}

bool HRootSignature::Build(HDirectXContext& DirectXContext, D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags)
{
	std::vector<CD3DX12_ROOT_PARAMETER1> D3DRootParameters{};
	for (HRootParameter& RootParameter : RootParameters)
	{
		CD3DX12_ROOT_PARAMETER1& D3DRootParameter = D3DRootParameters.emplace_back();

		D3D12_SHADER_VISIBILITY ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		switch (RootParameter.ShaderVisibility)
		{
			case HShaderVisibility::All:
				ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				break;
			case HShaderVisibility::Vertex:
				ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
				break;
			case HShaderVisibility::Pixel:
				ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
				break;
		}

		switch (RootParameter.RootParameterType)
		{
			case HRootParameterType::SRV:
			case HRootParameterType::UAV:
			case HRootParameterType::CBV:
			{
				D3DRootParameter.InitAsDescriptorTable(
					1,
					DescriptorRanges.data() + RootParameter.DescriptorRangeOffset,
					ShaderVisibility);
			}
			break;
			case HRootParameterType::Unknown:
			default:
				break;
		}
	}

	// Static Sampler
	D3D12_STATIC_SAMPLER_DESC StaticSampler = {};
	StaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	StaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	StaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	StaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	StaticSampler.MipLODBias = 0;
	StaticSampler.MaxAnisotropy = 0;
	StaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	StaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	StaticSampler.MinLOD = 0.0f;
	StaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	StaticSampler.ShaderRegister = 0;
	StaticSampler.RegisterSpace = 0;
	StaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc;

	RootSignatureDesc.Init_1_1(
		static_cast<UINT>(D3DRootParameters.size()),
		D3DRootParameters.data(),
		1,
		&StaticSampler,
		RootSignatureFlags);

	if (!HDirectX::CreateRootSignature(RootSiganature, RootSignatureDesc, DirectXContext.Device))
	{
		return false;
	}

	return true;
}