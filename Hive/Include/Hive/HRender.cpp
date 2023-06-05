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

	const ImGuiIO& IO = ImGui::GetIO();

	float CameraSpeed = 1.0f;
	if (IO.KeyShift)
	{
		CameraSpeed *= 10.0f;
	}
	HRelativeTransform& CameraTransform = Scene.Get<HRelativeTransform>(CameraEntity);
	if (IO.KeysDown['W'])
	{
		CameraTransform.Translation.z += CameraSpeed * IO.DeltaTime;
	}
	if (IO.KeysDown['S'])
	{
		CameraTransform.Translation.z -= CameraSpeed * IO.DeltaTime;
	}
	if (IO.KeysDown['D'])
	{
		CameraTransform.Translation.x += CameraSpeed * IO.DeltaTime;
	}
	if (IO.KeysDown['A'])
	{
		CameraTransform.Translation.x -= CameraSpeed * IO.DeltaTime;
	}
	if (IO.KeysDown['Q'])
	{
		CameraTransform.Translation.y += CameraSpeed * IO.DeltaTime;
	}
	if (IO.KeysDown['E'])
	{
		CameraTransform.Translation.y -= CameraSpeed * IO.DeltaTime;
	}

	RenderComputePass(GUIWindow, RenderWindow.ComputePass, Scene, CameraEntity, Resolution);

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


	if (!HDirectX::CreateCommandAllocator(
		&ComputePass.UpdateCommandAllocator,
		GUIWindow.DirectXContext->Device,
		D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE))
	{
		return false;
	}

	if (!HDirectX::CreateCommandList(
		&ComputePass.UpdateCommandList,
		ComputePass.UpdateCommandAllocator,
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

	ComputePass.Resolution = { 1024, 1024 };

	// Output
	{
		if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				&ComputePass.OutputResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Output", HRootParameterType::UAV);
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.OutputHeapIndex,
				ComputePass.OutputResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Spheres
	{
		uint64_t SphereCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				&ComputePass.SpheresResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HRenderedSphere),
				SphereCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Spheres", HRootParameterType::UAV);
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.SpheresHeapIndex,
				ComputePass.SpheresResource,
				SphereCount,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Materials
	{
		uint64_t MaterialCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				&ComputePass.MaterialsResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HMaterial),
				MaterialCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Materials", HRootParameterType::UAV);
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.MaterialsHeapIndex,
				ComputePass.MaterialsResource,
				MaterialCount,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Scene
	{
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				&ComputePass.SceneResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HRenderedScene),
				1))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Scene", HRootParameterType::UAV);
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.SceneHeapIndex,
				ComputePass.SceneResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// SDFs
	{
		uint64_t SDFCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				&ComputePass.SDFsResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HSDF),
				SDFCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("SDFs", HRootParameterType::UAV);
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.SDFsHeapIndex,
				ComputePass.SDFsResource,
				SDFCount,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Distance
	/*{
		// March
		ComputePass.RootSignature.AddRootParameter("MarchDistance", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				&ComputePass.MarchDistanceResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution,
				DXGI_FORMAT_R32_UINT))
		{
			return false;
		}
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.MarchDistanceHeapIndex,
				ComputePass.MarchDistanceResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}

		// Step
		ComputePass.RootSignature.AddRootParameter("StepDistance", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				&ComputePass.StepDistanceResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution,
				DXGI_FORMAT_R32_UINT))
		{
			return false;
		}
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.StepDistanceHeapIndex,
				ComputePass.StepDistanceResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}*/

	ComputePass.RootSignature.Build(GUIWindow);

	if (!HDirectX::CreateComputePipelineState(
			ComputePass.PipelineState,
			"Shaders/Hive/HComputeRender.hlsl",
			"main",
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
	entt::entity CameraEntity,
	const glm::vec2& Resolution)
{
	if (ComputePass.Resolution != Resolution)
	{
		if (ComputePass.RenderFuture.valid())
		{
			ComputePass.RenderFuture.wait();
		}
		HDirectX::WaitForFence(ComputePass.Fence, ComputePass.FenceValue);

		ComputePass.Resolution = Resolution;
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
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}

		// March
		/*if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				&ComputePass.MarchDistanceResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution,
				DXGI_FORMAT_R32_UINT))
		{
			return false;
		}
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.MarchDistanceHeapIndex,
				ComputePass.MarchDistanceResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}

		// Step
		if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				&ComputePass.StepDistanceResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution,
				DXGI_FORMAT_R32_UINT))
		{
			return false;
		}
		if (!ComputePass.CBVSRVUAVDescriptorHeap.CreateOrUpdateHandle(
				ComputePass.StepDistanceHeapIndex,
				ComputePass.StepDistanceResource,
				1,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}*/
	}

	if (ComputePass.TriggerShaderRebuild.exchange(false))
	{
		if (ComputePass.RenderFuture.valid())
		{
			ComputePass.RenderFuture.wait();
		}
		HDirectX::WaitForFence(ComputePass.Fence, ComputePass.FenceValue);

		if (!HDirectX::CreateComputePipelineState(
				ComputePass.PipelineState,
				"Shaders/Hive/HComputeRender.hlsl",
				"main",
				ComputePass.RootSignature.RootSiganature.Get(),
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	/*if (ComputePass.RenderFuture.valid() && ComputePass.RenderFuture.wait_for(std::chrono::milliseconds(0)) !=
	std::future_status::ready)
	{
		return true;
	}*/

	if (ComputePass.RenderFuture.valid())
	{
		ComputePass.RenderFuture.wait();
	}

	// Update Resources
	{
		ComputePass.UpdateCommandAllocator->Reset();
		ComputePass.UpdateCommandList->Reset(ComputePass.UpdateCommandAllocator, nullptr);

		uint64_t RenderSphersDataSize = Scene.RenderedSpheres.size() * sizeof(HRenderedSphere);
		HDirectX::CopyDataToResource(
			ComputePass.SpheresResource,
			ComputePass.SpheresUploadResource,
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			Scene.RenderedSpheres.data(),
			RenderSphersDataSize);

		uint64_t RenderedMaterialsDataSize = Scene.RenderedMaterials.size() * sizeof(HMaterial);
		HDirectX::CopyDataToResource(
			ComputePass.MaterialsResource,
			ComputePass.MaterialsUploadResource,
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			Scene.RenderedMaterials.data(),
			RenderedMaterialsDataSize);

		HRenderedScene RenderedScene{};
		RenderedScene.SphereCount = Scene.RenderedSpheres.size();
		RenderedScene.SDFCount = Scene.RenderedSDFs.size();
		const HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);
		RenderedScene.RayOrigin = CameraTransform.Translation;

		HDirectX::CopyDataToResource(
			ComputePass.SceneResource,
			ComputePass.SceneUploadResource,
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			&RenderedScene,
			sizeof(HRenderedScene));

		uint64_t SDFsDataSize = Scene.RenderedSDFs.size() * sizeof(HSDF);
		assert(SDFsDataSize > 0);
		HDirectX::CopyDataToResource(
			ComputePass.SDFsResource,
			ComputePass.SDFsUploadResource,
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			Scene.RenderedSDFs.data(),
			SDFsDataSize);

		ComputePass.UpdateCommandList->Close();
		HDirectX::ExecuteCommandLists<1>(ComputePass.CommandQueue, { ComputePass.UpdateCommandList });
	}

	ComputePass.RenderFuture = std::async(
		[&]() {
			ComputePass.CommandAllocator->Reset();
			ComputePass.CommandList->Reset(ComputePass.CommandAllocator, nullptr);

			// Set Root Signature
			{
				ComputePass.CommandList->SetDescriptorHeaps(1, &ComputePass.CBVSRVUAVDescriptorHeap.DescriptorHeap);
				ComputePass.CommandList->SetComputeRootSignature(ComputePass.RootSignature.RootSiganature.Get());
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					0,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.OutputHeapIndex));
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					1,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.SpheresHeapIndex));
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					2,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.MaterialsHeapIndex));
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					3,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.SceneHeapIndex));
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					4,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.SDFsHeapIndex));
				/*ComputePass.CommandList->SetComputeRootDescriptorTable(
					5,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.MarchDistanceHeapIndex));
				ComputePass.CommandList->SetComputeRootDescriptorTable(
					6,
					ComputePass.CBVSRVUAVDescriptorHeap.GetGPUHandle(ComputePass.StepDistanceHeapIndex));*/
			}

			// Draw Pass
			{
				ComputePass.CommandList->SetPipelineState(ComputePass.PipelineState.Get());
				ComputePass.CommandList->Dispatch(ComputePass.Resolution.x, ComputePass.Resolution.y, 1);
			}

			ComputePass.CommandList->Close();
			HDirectX::ExecuteCommandLists<1>(ComputePass.CommandQueue, { ComputePass.CommandList });

			ComputePass.FenceValue++;
			HDirectX::SignalFence(ComputePass.CommandQueue, ComputePass.Fence, ComputePass.FenceValue);
			HDirectX::WaitForFence(ComputePass.Fence, ComputePass.FenceValue);

			return true;
		});

	return true;
}

void HHoney::ControlsWindow(
	HGUIWindow& GUIWindow,
	HScene& Scene,
	entt::entity CameraEntity,
	HRenderWindow& RenderWindow)
{
	if (ImGui::BeginChild("Renderer"))
	{
		if (ImGui::Button("Rebuild Shaders"))
		{
			RenderWindow.ComputePass.TriggerShaderRebuild = true;
		}

		HRelativeTransform& CameraTransform = Scene.Get<HRelativeTransform>(CameraEntity);
		ImGui::DragScalarN("Ray Origin", ImGuiDataType_Double, &CameraTransform.Translation.x, 3);
	}
	ImGui::EndChild();
}