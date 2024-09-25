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

void HRootSignature::AddRootParameter(
	std::string_view Name,
	HRootParameterType RootParameterType,
	HShaderVisibility ShaderVisibility)
{
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
			break;
	}
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

bool HHoney::CreatRenderPass(HDirectXContext& DirectXContext, HRenderPass& RenderPass, const glm::vec2& Resolution)
{
	if (!HDirectX::CreateCommandAllocator(
			&RenderPass.CommandAllocator,
			DirectXContext.Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		assert(false);
		return false;
	}

	if (!HDirectX::CreateCommandList(
			&RenderPass.CommandList,
			RenderPass.CommandAllocator,
			DirectXContext.Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		assert(false);
		return false;
	}

	if (!HDirectX::CreateCommandAllocator(
			&RenderPass.UpdateCommandAllocator,
			DirectXContext.Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		assert(false);
		return false;
	}

	if (!HDirectX::CreateCommandList(
			&RenderPass.UpdateCommandList,
			RenderPass.UpdateCommandAllocator,
			DirectXContext.Device,
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	if (!HDirectX::CreateFence(RenderPass.Fence, DirectXContext.Device))
	{
		assert(false);
		return false;
	}

	if (!HDirectX::CreateCBVSRVUAVHeap(RenderPass.CBVSRVUAVDescriptorHeap, DirectXContext.Device, 1000000))
	{
		assert(false);
		return false;
	}

	if (!HDirectX::CreateRTVHeap(RenderPass.RTVDescriptorHeap, DirectXContext.Device, 8))
	{
		assert(false);
		return false;
	}

	for (uint64_t OutputResource = 0; OutputResource < HRenderPass::OutputBufferCount; OutputResource++)
	{
		// Output Resources
		{
			if (!HDirectX::CreateOrUpdateUnorderedTextureResource(
					RenderPass.OutputResources[OutputResource],
					DirectXContext.Device,
					Resolution,
					DXGI_FORMAT_R8G8B8A8_UNORM,
					true))
			{
				assert(false);
				return false;
			}
			if (!HDirectX::CreateOrUpdateRTV(
					RenderPass.OutputRTVDescriptors[OutputResource],
					RenderPass.OutputResources[OutputResource].Resource,
					RenderPass.RTVDescriptorHeap,
					DirectXContext.Device))
			{
				assert(false);
				return false;
			}
		}

		// Scene Resources
		{
			if (!HDirectX::CreateOrUpdateUploadBufferResource(
					RenderPass.SceneBufferResources[OutputResource],
					DirectXContext.Device,
					sizeof(HSceneBuffer)))
			{
				assert(false);
				return false;
			}
			if (!HDirectX::CreateOrUpdateCBV(
					RenderPass.SceneBufferDescriptors[OutputResource],
					RenderPass.SceneBufferResources[OutputResource],
					0,
					HDirectX::CalculateAlignedSize(sizeof(HSceneBuffer), 256),
					RenderPass.CBVSRVUAVDescriptorHeap,
					DirectXContext.Device))
			{
				assert(false);
				return false;
			}
			CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
			if (!CheckResult(RenderPass.SceneBufferResources[OutputResource].Resource->Map(
					0,
					&ReadRange,
					reinterpret_cast<void**>(&RenderPass.MappedSceneBuffers[OutputResource]))))
			{
				assert(false);
				return false;
			}
		}

		// Instance Resources
		{
			RenderPass.InstanceBufferResources[OutputResource].resize(1);
			if (!HDirectX::CreateOrUpdateUploadBufferResource(
					RenderPass.InstanceBufferResources[OutputResource][0],
					DirectXContext.Device,
					sizeof(HInstanceBuffer)))
			{
				assert(false);
				return false;
			}
			RenderPass.MappedInstanceBuffers[OutputResource].resize(1);
			CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
			if (!CheckResult(RenderPass.InstanceBufferResources[OutputResource][0].Resource->Map(
					0,
					&ReadRange,
					reinterpret_cast<void**>(&RenderPass.MappedInstanceBuffers[OutputResource][0]))))
			{
				assert(false);
				return false;
			}
			RenderPass.InstanceBufferDescriptors[OutputResource].resize(1);
			if (!HDirectX::CreateOrUpdateCBV(
					RenderPass.InstanceBufferDescriptors[OutputResource][0],
					RenderPass.InstanceBufferResources[OutputResource][0],
					0,
					HDirectX::CalculateAlignedSize(sizeof(HInstanceBuffer), 256),
					RenderPass.CBVSRVUAVDescriptorHeap,
					DirectXContext.Device))
			{
				assert(false);
				return false;
			}
		}
	}

	RenderPass.RootSignature.AddRootParameter("SceneBuffer", HRootParameterType::CBV, HShaderVisibility::Vertex);
	RenderPass.RootSignature.AddRootParameter("Texture", HRootParameterType::SRV, HShaderVisibility::Pixel);
	RenderPass.RootSignature.AddRootParameter("InstanceBuffer", HRootParameterType::CBV, HShaderVisibility::Vertex);
	RenderPass.RootSignature.AddRootParameter("ModelBuffer", HRootParameterType::CBV, HShaderVisibility::Vertex);
	RenderPass.RootSignature.Build(DirectXContext, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> VSBlob;
	if (!HDirectX::CompileShader("Shaders/Hive/HRenderPass.hlsl", "VSMain", "vs_5_0", VSBlob))
	{
		assert(false);
		return false;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> PSBlob;
	if (!HDirectX::CompileShader("Shaders/Hive/HRenderPass.hlsl", "PSMain", "ps_5_0", PSBlob))
	{
		assert(false);
		return false;
	}

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC InputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateObjectDesc = {};
	PipelineStateObjectDesc.InputLayout = { InputElementDescs, _countof(InputElementDescs) };
	PipelineStateObjectDesc.pRootSignature = RenderPass.RootSignature.RootSiganature.Get();
	PipelineStateObjectDesc.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
	PipelineStateObjectDesc.PS = CD3DX12_SHADER_BYTECODE(PSBlob.Get());
	PipelineStateObjectDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PipelineStateObjectDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PipelineStateObjectDesc.DepthStencilState.DepthEnable = FALSE;
	PipelineStateObjectDesc.DepthStencilState.StencilEnable = FALSE;
	PipelineStateObjectDesc.SampleMask = UINT_MAX;
	PipelineStateObjectDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PipelineStateObjectDesc.NumRenderTargets = 1;
	PipelineStateObjectDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateObjectDesc.SampleDesc.Count = 1;
	HRESULT Result = DirectXContext.Device->CreateGraphicsPipelineState(
		&PipelineStateObjectDesc,
		IID_PPV_ARGS(&RenderPass.PipelineState));
	if (!SUCCEEDED(Result))
	{
		assert(false);
		return false;
	}

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		HVertex TriangleVertices[] = { { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.0f } },
									   { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f } },
									   { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f } } };

		uint64_t VertexBufferSize = sizeof(TriangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not
		// recommended. Every time the GPU needs it, the upload heap will be marshalled
		// over. Please read up on Default Heap usage. An upload heap is used here for
		// code simplicity and because there are very few verts to actually transfer.
		CD3DX12_RESOURCE_DESC VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		CD3DX12_HEAP_PROPERTIES UploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		Result = DirectXContext.Device->CreateCommittedResource(
			&UploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&VertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&RenderPass.VertexBufferResource));
		if (!SUCCEEDED(Result))
		{
			assert(false);
			return false;
		}

		// Map the GPU buffer so we can write to it
		CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
		Result =
			RenderPass.VertexBufferResource->Map(0, &ReadRange, reinterpret_cast<void**>(&RenderPass.VertexBufferData));
		if (!SUCCEEDED(Result))
		{
			assert(false);
			return false;
		}
		memcpy(RenderPass.VertexBufferData, TriangleVertices, sizeof(TriangleVertices));

		// Initialize the vertex buffer view.
		RenderPass.VertexBufferView.BufferLocation = RenderPass.VertexBufferResource->GetGPUVirtualAddress();
		RenderPass.VertexBufferView.StrideInBytes = sizeof(HVertex);
		RenderPass.VertexBufferView.SizeInBytes = VertexBufferSize;
	}

	return true;
}

bool HHoney::RenderRenderPass(
	HDirectXContext& DirectXContext,
	HRenderPass& RenderPass,
	const glm::vec3& Translation,
	const glm::vec3& Scale,
	const HMesh& Mesh,
	const std::vector<glm::vec3>& Transforms,
	const HTexture& Texture,
	const glm::vec2& Resolution)
{
	// Swap Buffers
	RenderPass.FrontBufferIndex = (RenderPass.FrontBufferIndex + 1) % HRenderPass::OutputBufferCount;
	uint64_t BackBufferIndex = (RenderPass.FrontBufferIndex + 1) % HRenderPass::OutputBufferCount;

	// Update Scene
	RenderPass.MappedSceneBuffers[BackBufferIndex]->Translation = glm::vec4(Translation, 0.0f);
	RenderPass.MappedSceneBuffers[BackBufferIndex]->Scale = glm::vec4(Scale, 0.0f);

	// Update Mesh Buffers
	{
		uint64_t VertexBufferSize = 0;
		//for (const HMesh& Mesh : Meshes)
		{
			VertexBufferSize += sizeof(HVertex) * Mesh.Verticies.size();
		}
		if (VertexBufferSize > 0)
		{
			// Resize the vertex buffer
			if (RenderPass.VertexBufferView.SizeInBytes != VertexBufferSize)
			{
				RenderPass.VertexBufferResource->Unmap(0, nullptr);
				// Note: using upload heaps to transfer static data like vert buffers is not
				// recommended. Every time the GPU needs it, the upload heap will be marshalled
				// over. Please read up on Default Heap usage. An upload heap is used here for
				// code simplicity and because there are very few verts to actually transfer.
				CD3DX12_RESOURCE_DESC VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
				CD3DX12_HEAP_PROPERTIES UploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
				if (!CheckResult(DirectXContext.Device->CreateCommittedResource(
						&UploadHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&VertexBufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&RenderPass.VertexBufferResource))))
				{
					return false;
				}

				// Map the GPU buffer so we can write to it
				CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
				if (!CheckResult(RenderPass.VertexBufferResource
									 ->Map(0, &ReadRange, reinterpret_cast<void**>(&RenderPass.VertexBufferData))))
				{
					return false;
				}

				// Initialize the vertex buffer view.
				RenderPass.VertexBufferView.BufferLocation = RenderPass.VertexBufferResource->GetGPUVirtualAddress();
				RenderPass.VertexBufferView.StrideInBytes = sizeof(HVertex);
				RenderPass.VertexBufferView.SizeInBytes = VertexBufferSize;
			}
		}

		// Resize the instance Buffers
		{
			uint64_t InstanceBufferAlignedSize = HDirectX::CalculateAlignedSize(sizeof(HInstanceBuffer), 256);
			uint64_t OldInstanceBufferCount = RenderPass.InstanceBufferResources[BackBufferIndex].size();
			if (OldInstanceBufferCount < Transforms.size())
			{
				RenderPass.InstanceBufferResources[BackBufferIndex].resize(Transforms.size());
				RenderPass.MappedInstanceBuffers[BackBufferIndex].resize(Transforms.size());
				RenderPass.InstanceBufferDescriptors[BackBufferIndex].resize(Transforms.size());

				for (uint64_t MeshIndex = OldInstanceBufferCount; MeshIndex < Transforms.size(); MeshIndex++)
				{
					// Create Resources
					if (!HDirectX::CreateOrUpdateUploadBufferResource(
							RenderPass.InstanceBufferResources[BackBufferIndex][MeshIndex],
							DirectXContext.Device,
							InstanceBufferAlignedSize))
					{
						return false;
					}

					// Map Resources
					CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
					if (!CheckResult(RenderPass.InstanceBufferResources[BackBufferIndex][MeshIndex].Resource->Map(
							0,
							&ReadRange,
							reinterpret_cast<void**>(&RenderPass.MappedInstanceBuffers[BackBufferIndex][MeshIndex]))))
					{
						return false;
					}

					// Create CBV
					if (!HDirectX::CreateOrUpdateCBV(
							RenderPass.InstanceBufferDescriptors[BackBufferIndex][MeshIndex],
							RenderPass.InstanceBufferResources[BackBufferIndex][MeshIndex],
							0,
							InstanceBufferAlignedSize,
							RenderPass.CBVSRVUAVDescriptorHeap,
							DirectXContext.Device))
					{
						return false;
					}
				}
			}
		}

		// Update the Mesh Data
		{
			memcpy(
				RenderPass.VertexBufferData,
				Mesh.Verticies.data(),
				sizeof(HVertex) * Mesh.Verticies.size());
		}

		// Update the instance buffers
		uint64_t VertexBufferOffset = 0;
		for (size_t TransformIndex = 0; TransformIndex < Transforms.size(); TransformIndex++)
		{
			glm::vec4 Transltion = glm::vec4(Transforms[TransformIndex], 0.0f);

			RenderPass.MappedInstanceBuffers[BackBufferIndex][TransformIndex]->Translation = Transltion;
		}
	}

	// Command list allocators can only be reset when the associated
	// command lists have finished execution on the GPU; apps should use
	// fences to determine GPU execution progress.
	if (!CheckResult(RenderPass.CommandAllocator->Reset()))
		return false;

	// However, when ExecuteCommandList() is called on a particular command
	// list, that command list can then be reset at any time and must be before
	// re-recording.
	if (!CheckResult(RenderPass.CommandList->Reset(RenderPass.CommandAllocator, RenderPass.PipelineState.Get())))
		return false;

	// Set necessary state.
	RenderPass.CommandList->SetGraphicsRootSignature(RenderPass.RootSignature.RootSiganature.Get());

	ID3D12DescriptorHeap* Heaps[] = { RenderPass.CBVSRVUAVDescriptorHeap.DescriptorHeap };
	RenderPass.CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

	RenderPass.CommandList->SetGraphicsRootDescriptorTable(
		0,
		RenderPass.SceneBufferDescriptors[BackBufferIndex].GPUDescriptorHandle);

	if (RenderPass.TextureDescriptors.empty())
	{
		HDescriptor& TextureDescriptor = RenderPass.TextureDescriptors.emplace_back();
		HDirectX::CreateOrUpdateSRV(
			TextureDescriptor,
			Texture.Resource.Resource,
			RenderPass.CBVSRVUAVDescriptorHeap,
			DirectXContext.Device);
	}
	RenderPass.CommandList->SetGraphicsRootDescriptorTable(1, RenderPass.TextureDescriptors[0].GPUDescriptorHandle);

	CD3DX12_VIEWPORT Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, Resolution.x, Resolution.y);
	CD3DX12_RECT ScissorRect = CD3DX12_RECT(0, 0, Resolution.x, Resolution.y);

	RenderPass.CommandList->RSSetViewports(1, &Viewport);
	RenderPass.CommandList->RSSetScissorRects(1, &ScissorRect);

	// Indicate that the back buffer will be used as a render target.
	D3D12_RESOURCE_BARRIER StartBarriers[] = { CD3DX12_RESOURCE_BARRIER::Transition(
		RenderPass.OutputResources[BackBufferIndex].Resource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET) };
	RenderPass.CommandList->ResourceBarrier(_countof(StartBarriers), StartBarriers);
	RenderPass.CommandList
		->OMSetRenderTargets(1, &RenderPass.OutputRTVDescriptors[BackBufferIndex].CPUDescriptorHandle, FALSE, nullptr);

	// Record commands.
	const float ClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	RenderPass.CommandList->ClearRenderTargetView(
		RenderPass.OutputRTVDescriptors[BackBufferIndex].CPUDescriptorHandle,
		ClearColor,
		0,
		nullptr);
	RenderPass.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	RenderPass.CommandList->IASetVertexBuffers(0, 1, &RenderPass.VertexBufferView);

	RenderPass.CommandList->SetGraphicsRootDescriptorTable(
		2,
		RenderPass.InstanceBufferDescriptors[BackBufferIndex][0].GPUDescriptorHandle);

	if (!Transforms.empty())
	{
		RenderPass.CommandList->DrawInstanced(glm::max(3ull, Mesh.Verticies.size()), Transforms.size(), 0, 0);
	}

	D3D12_RESOURCE_BARRIER EndBarriers[] = { CD3DX12_RESOURCE_BARRIER::Transition(
		RenderPass.OutputResources[BackBufferIndex].Resource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
	RenderPass.CommandList->ResourceBarrier(_countof(EndBarriers), EndBarriers);

	if (!CheckResult(RenderPass.CommandList->Close()))
		return false;

	ID3D12CommandList* CommandLists[] = { RenderPass.CommandList };
	DirectXContext.CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	// This might not work for more than 2 buffers
	static_assert(HRenderPass::OutputBufferCount == 2);
	HDirectX::SignalFence(DirectXContext.CommandQueue, RenderPass.Fence, RenderPass.FenceValue);

	// Render synchronously for now
	// Move this to the top once we figure out resource access
	// Check to see if we have finished rendering to the back buffer
	if (RenderPass.FenceValue > 0)
	{
		HDirectX::WaitForFence(RenderPass.Fence, RenderPass.FenceValue);
		// if (!HDirectX::CheckFenceComplete(RenderPass.Fence, RenderPass.FenceValue))
		/*{
			return true;
		}*/
	}

	return true;
}

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

	if (!HDirectX::CreateFence(ComputePass.Fence, GUIWindow.DirectXContext->Device))
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
