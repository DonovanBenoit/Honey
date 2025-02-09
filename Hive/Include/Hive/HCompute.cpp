#include "HCompute.h"

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


bool HHoney::CreatComputePass(HGUIWindow& GUIWindow, HComputePass& ComputePass)
{
	if (!HDirectX::CreateCommandQueue(
			&ComputePass.CommandQueue,
			GUIWindow.DirectXContext->Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,
			"Compute"))
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
	ComputePass.Fence = HDirectX::CreateFence(GUIWindow.DirectXContext->Device);
	if (!ComputePass.Fence)
	{
		return false;
	}

	if (!HDirectX::CreateCBVSRVUAVHeap(ComputePass.CBVSRVUAVDescriptorHeap, GUIWindow.DirectXContext->Device, 1000000))
	{
		return false;
	}

	ComputePass.Resolution = { 1024, 1024 };

	// Output
	{
		if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
				ComputePass.OutputResource,
				GUIWindow.DirectXContext->Device,
				ComputePass.Resolution))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Output", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUAV(
				ComputePass.OutputDescriptor,
				ComputePass.OutputResource.Resource,
				1,
				ComputePass.CBVSRVUAVDescriptorHeap,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Spheres
	{
		uint64_t SphereCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				ComputePass.SpheresResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HRenderedSphere),
				SphereCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Spheres", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUAV(
				ComputePass.SpheresDescriptor,
				ComputePass.SpheresResource.Resource,
				SphereCount,
				ComputePass.CBVSRVUAVDescriptorHeap,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Materials
	{
		uint64_t MaterialCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				ComputePass.MaterialsResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HMaterial),
				MaterialCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Materials", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUAV(
				ComputePass.MaterialsDescriptor,
				ComputePass.MaterialsResource.Resource,
				MaterialCount,
				ComputePass.CBVSRVUAVDescriptorHeap,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// Scene
	{
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				ComputePass.SceneResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HRenderedScene),
				1))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("Scene", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUAV(
				ComputePass.SceneDescriptor,
				ComputePass.SceneResource.Resource,
				1,
				ComputePass.CBVSRVUAVDescriptorHeap,
				GUIWindow.DirectXContext->Device))
		{
			return false;
		}
	}

	// SDFs
	{
		uint64_t SDFCount = 1024;
		if (!HDirectX::CreateOrUpdateUnorderedBufferResource(
				ComputePass.SDFsResource,
				GUIWindow.DirectXContext->Device,
				sizeof(HSDF),
				SDFCount))
		{
			return false;
		}
		ComputePass.RootSignature.AddRootParameter("SDFs", HRootParameterType::UAV);
		if (!HDirectX::CreateOrUpdateUAV(
				ComputePass.SDFsDescriptor,
				ComputePass.SDFsResource.Resource,
				SDFCount,
				ComputePass.CBVSRVUAVDescriptorHeap,
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

	ComputePass.RootSignature.Build(*GUIWindow.DirectXContext);

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
		/*if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
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
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			Scene.RenderedSpheres.data(),
			RenderSphersDataSize);

		uint64_t RenderedMaterialsDataSize = Scene.RenderedMaterials.size() * sizeof(HMaterial);
		HDirectX::CopyDataToResource(
			ComputePass.MaterialsResource,
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
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			&RenderedScene,
			sizeof(HRenderedScene));

		uint64_t SDFsDataSize = Scene.RenderedSDFs.size() * sizeof(HSDF);
		assert(SDFsDataSize > 0);
		HDirectX::CopyDataToResource(
			ComputePass.SDFsResource,
			GUIWindow.DirectXContext->Device,
			ComputePass.UpdateCommandList,
			Scene.RenderedSDFs.data(),
			SDFsDataSize);

		ComputePass.UpdateCommandList->Close();
		HDirectX::ExecuteCommandLists<1>(ComputePass.CommandQueue, { ComputePass.UpdateCommandList });
	}

	ComputePass.RenderFuture = std::async([&]() {
		ComputePass.CommandAllocator->Reset();
		ComputePass.CommandList->Reset(ComputePass.CommandAllocator, nullptr);

		// Set Root Signature
		{
			ComputePass.CommandList->SetDescriptorHeaps(1, &ComputePass.CBVSRVUAVDescriptorHeap.DescriptorHeap);
			ComputePass.CommandList->SetComputeRootSignature(ComputePass.RootSignature.RootSiganature.Get());
			ComputePass.CommandList->SetComputeRootDescriptorTable(0, ComputePass.OutputDescriptor.GPUDescriptorHandle);
			ComputePass.CommandList->SetComputeRootDescriptorTable(
				1,
				ComputePass.SpheresDescriptor.GPUDescriptorHandle);
			ComputePass.CommandList->SetComputeRootDescriptorTable(
				2,
				ComputePass.MaterialsDescriptor.GPUDescriptorHandle);
			ComputePass.CommandList->SetComputeRootDescriptorTable(3, ComputePass.SceneDescriptor.GPUDescriptorHandle);
			ComputePass.CommandList->SetComputeRootDescriptorTable(4, ComputePass.SDFsDescriptor.GPUDescriptorHandle);
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
