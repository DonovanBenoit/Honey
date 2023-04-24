#include "HRender.h"

#include "HScene.h"
#include "HWindow.h"

#include <HImGui.h>
#include <chrono>
#include <entt/entt.hpp>
#include <format>
#include <future>
#include <glm/gtx/intersect.hpp>
#include <imgui.h>
#include <thread>
#include <vector>

namespace
{
	bool RaySphereIntersect(HRenderedSphere& RenderedSphere, const glm::vec3& RayDirection, float& IntersectionDistance)
	{
		constexpr float Epsilon = std::numeric_limits<float>::epsilon();

		float t0 = glm::dot(RenderedSphere.RayOriginToSphereCenter, RayDirection);
		float d2 = glm::dot(RenderedSphere.RayOriginToSphereCenter, RenderedSphere.RayOriginToSphereCenter) - t0 * t0;
		if (d2 > RenderedSphere.RadiusSquared)
		{
			// The ray misses the sphere
			return false;
		}

		float t1 = glm::sqrt(RenderedSphere.RadiusSquared - d2);
		IntersectionDistance = t0 > t1 + Epsilon ? t0 - t1 : t0 + t1;
		return IntersectionDistance > Epsilon;
	}

	bool TraceRay(
		HScene& Scene,
		const glm::vec3& RayOrigin,
		const glm::vec3& RayDirection,
		glm::vec3& HitPosition,
		glm::vec3& HitNormal,
		glm::vec3& Albedo)
	{
		float FarClip = 1000.0f;
		float THit = FarClip;

		entt::entity HitSphereEntity = entt::null;

		for (HRenderedSphere& RenderedSphere : Scene.RenderedSpheres)
		{
			float SphereHit = 0.0f;
			if (RaySphereIntersect(RenderedSphere, RayDirection, SphereHit))
			{
				if (SphereHit < THit)
				{
					THit = SphereHit;
				}
			}
		}

		if (HitSphereEntity != entt::null)
		{
			HMaterial& Material = Scene.Get<HMaterial>(HitSphereEntity);
			HSphere& Sphere = Scene.Get<HSphere>(HitSphereEntity);
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(HitSphereEntity);
			HitPosition = RayOrigin + RayDirection * THit;
			HitNormal = (HitPosition - WorldTransform.Translation) / Sphere.Radius;
			Albedo = Material.Albedo;
		}

		return THit < FarClip;
	}

	glm::vec3 Lighting(HScene& Scene, const glm::vec3& WorldPosition, const glm::vec3& WorldNormal)
	{
		glm::vec3 Color = {};

		auto PointLightView = Scene.Registry.view<HPointLight>();
		for (entt::entity PointLightEntity : PointLightView)
		{
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(PointLightEntity);
			HPointLight& PointLight = Scene.Get<HPointLight>(PointLightEntity);

			const glm::vec3 Displacement = WorldTransform.Translation - WorldPosition;
			float Distance = glm::length(Displacement);
			if (Distance <= 0.0f)
			{
				continue;
			}
			float LightDot = glm::max(glm::dot(Displacement / Distance, WorldNormal), 0.0f);
			float DistanceFromSurface = Distance - PointLight.Radius + 1.0f;
			if (DistanceFromSurface < 1.0f)
			{
				Color += PointLight.Color * LightDot;
			}
			else
			{
				Color += PointLight.Color * LightDot / (DistanceFromSurface * DistanceFromSurface);
			}
		}

		return Color;
	}

	uint64_t Ceil(uint64_t Dividend, uint64_t Divisior)
	{
		return (Dividend + Divisior - 1) / Divisior;
	}

} // namespace

void HHoney::Render(HScene& Scene, HGUIImage& Image, entt::entity CameraEntity)
{
	const HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
	const HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

	const uint64_t Size = Image.Width * Image.Height;
	static std::vector<glm::vec3> RayDirections;
	if (RayDirections.size() != Image.Width * Image.Height)
	{
		RayDirections.clear();
		const float OneOverWidth = 1.0f / float(Image.Width);
		const float OneOverHeight = 1.0f / float(Image.Height);
		const float AspectRatio = float(Image.Height) / float(Image.Width);
		for (uint32_t Y = 0; Y < Image.Height; Y++)
		{
			for (uint32_t X = 0; X < Image.Width; X++)
			{
				RayDirections.emplace_back(glm::normalize(
					glm::vec3{ (float(X) + 0.5f - float(Image.Width) * 0.5f) * OneOverWidth,
							   (float(Y) + 0.5f - float(Image.Height) * 0.5f) * OneOverHeight * AspectRatio,
							   Camera.FocalLength }));
			}
		}
	}
	glm::vec3 RayOrigin = CameraTransform.Translation;
	uint32_t ThreadCount = glm::max<uint32_t>(std::thread::hardware_concurrency() - 1, 1);
	uint32_t RowsPerThread = Ceil(Image.Height, ThreadCount);
	std::vector<std::future<void>> RenderFutures;
	for (uint32_t Thread = 0; Thread < ThreadCount; Thread++)
	{
		RenderFutures.emplace_back(std::async(
			[&](uint32_t YStart, uint32_t YEnd) {
				for (uint32_t Y = YStart; Y < YEnd; Y++)
				{
					for (uint32_t X = 0; X < Image.Width; X++)
					{
						uint32_t Pixel = Y * Image.Width + X;
						glm::vec3 IntersectionPoint = {};
						glm::vec3 IntersectionNormal = {};
						glm::vec3 IntersectionAlbedo = {};
						if (TraceRay(
								Scene,
								RayOrigin,
								RayDirections[Pixel],
								IntersectionPoint,
								IntersectionNormal,
								IntersectionAlbedo))
						{
							Image.Pixels[Pixel] = ImGui::ColorConvertFloat4ToU32(glm::vec4(
								Lighting(Scene, IntersectionPoint, IntersectionNormal) * IntersectionAlbedo,
								1.0f));
						}
						else
						{
							Image.Pixels[Pixel] = 0xFFF5B05C;
						}
					}
				}
			},
			Thread * RowsPerThread,
			glm::min<uint32_t>((Thread + 1) * RowsPerThread, Image.Height)));
	}

	for (std::future<void>& RenderFuture : RenderFutures)
	{
		RenderFuture.wait();
	}
}

void HHoney::DrawRender(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow)
{
	std::chrono::time_point Start = std::chrono::high_resolution_clock::now();

	ImVec2 Resolution = ImGui::GetContentRegionAvail();
	if (!HImGui::CreateOrUpdateImage(
			GUIWindow,
			RenderWindow.ImageIndex,
			static_cast<uint64_t>(Resolution.x),
			static_cast<uint64_t>(Resolution.y)))
	{
		return;
	}

	RenderComputePass(GUIWindow, RenderWindow.ComputePass, Scene, Resolution);

	HGUIImage& Image = GUIWindow.Images[RenderWindow.ImageIndex];

	SIZE_T CBVSRVUAV_DescriptorSize =
		GUIWindow.DirectXContext->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE CBVSRVUAV_Handle = GUIWindow.CBVSRVUAV_DescHeap->GetCPUDescriptorHandleForHeapStart();
	CBVSRVUAV_Handle.ptr += (RenderWindow.ImageIndex + 1) * CBVSRVUAV_DescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptor{};
	SRVDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDescriptor.Texture2D.MipLevels = 1;
	GUIWindow.DirectXContext->Device->CreateShaderResourceView(
		RenderWindow.ComputePass.OutputResource,
		&SRVDescriptor,
		CBVSRVUAV_Handle);

	std::chrono::time_point End = std::chrono::high_resolution_clock::now();

	ImVec2 ScreenPosition = ImGui::GetCursorScreenPos();

	HImGui::DrawImage(GUIWindow, RenderWindow.ImageIndex);

	ImGui::GetWindowDrawList()->AddText(
		ScreenPosition,
		0xFFFFFFFF,
		std::format(
			"{}x{}  {:0.2f}ms {:0.2f}FPS",
			Image.Width,
			Image.Height,
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(End - Start).count(),
			ImGui::GetIO().Framerate)
			.c_str());
}

void HRootSignature::AddRootParameter(std::string_view Name, HRootParameterType RootParameterType)
{
	HRootParameter& RootParameter = RootParameters.emplace_back();
	RootParameter.Name = Name;
	RootParameter.RootParameterType = RootParameterType;
	switch (RootParameter.RootParameterType)
	{
		case HRootParameterType::UAV:
		{
			RootParameter.ShaderRegister = UAVRegisterCount;
			UAVRegisterCount++;
			RootParameter.DescriptorRangeOffset = DescriptorRanges.size();
			CD3DX12_DESCRIPTOR_RANGE& DescriptorRange = DescriptorRanges.emplace_back();
			DescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, RootParameter.ShaderRegister);
		}
		break;
		case HRootParameterType::Unknown:
		default:
			break;
	}
}

bool HRootSignature::Build(HGUIWindow& GUIWindow)
{
	std::vector<CD3DX12_ROOT_PARAMETER> D3DRootParameters{};
	for (HRootParameter& RootParameter : RootParameters)
	{
		CD3DX12_ROOT_PARAMETER& D3DRootParameter = D3DRootParameters.emplace_back();

		switch (RootParameter.RootParameterType)
		{
			case HRootParameterType::UAV:
			{
				D3DRootParameter.InitAsDescriptorTable(
					1,
					DescriptorRanges.data() + RootParameter.DescriptorRangeOffset);
			}
			break;
			case HRootParameterType::Unknown:
			default:
				break;
		}
	}

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;

	RootSignatureDesc.Init(
		D3DRootParameters.size(),
		D3DRootParameters.data(),
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	if (!HDirectX::CreateRootSignature(RootSiganature, RootSignatureDesc, GUIWindow.DirectXContext->Device))
	{
		return false;
	}

	return true;
}

bool HHoney::CreatComputePass(HGUIWindow& GUIWindow, HComputePass& ComputePass)
{
	if (!HDirectX::CreateCommandQueue(
			&ComputePass.CommandQueue,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE))
	{
		return false;
	}

	if (!HDirectX::CreateCommandAllocator(
			&ComputePass.CommandAllocator,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE))
	{
		return false;
	}

	if (!HDirectX::CreateCommandList(
			&ComputePass.CommandList,
			ComputePass.CommandAllocator,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE))
	{
		return false;
	}

	if (!HDirectX::CreateFence(ComputePass.Fence, GUIWindow.DirectXContext->Device))
	{
		return false;
	}

	if (!ComputePass.CBVSRVUAVDescriptorHeap.Create(GUIWindow.DirectXContext->Device))
	{
		return false;
	}

	if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
			&ComputePass.OutputResource,
			GUIWindow.DirectXContext->Device,
			{ 1024, 1024 }))
	{
		return false;
	}

	// Spheres
	{
		uint64_t SphereCount = 1024;
		if (!HDirectX::CreateUnorderedBufferResource(
				&ComputePass.SpheresResource,
				GUIWindow.DirectXContext->Device,
				sizeof(glm::vec4),
				SphereCount))
		{
			return false;
		}

		HDirectX::ExecuteCommandLists<1>(ComputePass.CommandQueue, { ComputePass.CommandList });
		ComputePass.FenceValue++;
		HDirectX::SignalFence(ComputePass.CommandQueue, ComputePass.Fence, ComputePass.FenceValue);
		HDirectX::WaitForFence(ComputePass.Fence, ComputePass.FenceValue);
	}

	ComputePass.RootSignature.AddRootParameter("Output", HRootParameterType::UAV);
	ComputePass.RootSignature.AddRootParameter("Spheres", HRootParameterType::UAV);

	if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
			ComputePass.OutputHeapIndex,
			ComputePass.OutputResource,
			GUIWindow.DirectXContext->Device))
	{
		return false;
	}
	if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
			ComputePass.SpheresHeapIndex,
			ComputePass.SpheresResource,
			GUIWindow.DirectXContext->Device))
	{
		return false;
	}

	ComputePass.RootSignature.Build(GUIWindow);

	if (!HDirectX::CreateComputePipelineState(
			ComputePass.PipelineState,
			ComputePass.RootSignature.RootSiganature.Get(),
			GUIWindow.DirectXContext->Device))
	{
		return false;
	}
	return true;
};

bool HHoney::RenderComputePass(
	HGUIWindow& GUIWindow,
	HComputePass& ComputePass,
	HScene& Scene,
	const glm::vec2& Resolution)
{
	if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
			&ComputePass.OutputResource,
			GUIWindow.DirectXContext->Device,
			Resolution))
	{
		return false;
	}
	if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
			ComputePass.OutputHeapIndex,
			ComputePass.OutputResource,
			GUIWindow.DirectXContext->Device))
	{
		return false;
	}

	ComputePass.CommandAllocator->Reset();
	ComputePass.CommandList->Reset(ComputePass.CommandAllocator, nullptr);

	uint64_t RenderSphersDataSize = Scene.RenderedSpheres.size() * sizeof(glm::vec4);
	HDirectX::CopyDataToResource(
		ComputePass.SpheresResource,
		GUIWindow.DirectXContext->Device,
		ComputePass.CommandList,
		Scene.RenderedSpheres.data(),
		RenderSphersDataSize);

	ComputePass.CommandList->SetDescriptorHeaps(1, &ComputePass.CBVSRVUAVDescriptorHeap.DescriptorHeap);
	ComputePass.CommandList->SetComputeRootSignature(ComputePass.RootSignature.RootSiganature.Get());
	ComputePass.CommandList->SetComputeRootDescriptorTable(
		0,
		ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.OutputHeapIndex));
	ComputePass.CommandList->SetComputeRootDescriptorTable(
		1,
		ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.SpheresHeapIndex));
	ComputePass.CommandList->SetPipelineState(ComputePass.PipelineState.Get());

	ComputePass.CommandList->Dispatch(Resolution.x, Resolution.y, 1);

	ComputePass.CommandList->Close();

	HDirectX::ExecuteCommandLists<1>(ComputePass.CommandQueue, { ComputePass.CommandList });

	ComputePass.FenceValue++;
	HDirectX::SignalFence(ComputePass.CommandQueue, ComputePass.Fence, ComputePass.FenceValue);
	HDirectX::WaitForFence(ComputePass.Fence, ComputePass.FenceValue);

	return true;
}