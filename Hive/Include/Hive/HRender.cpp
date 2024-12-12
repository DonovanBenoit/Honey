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
			if (!HDirectX::CreateOrUpdateUploadBufferResource(
					RenderPass.InstanceBufferResources[OutputResource],
					DirectXContext.Device,
					sizeof(glm::vec4)))
			{
				assert(false);
				return false;
			}
			CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
			if (!CheckResult(RenderPass.InstanceBufferResources[OutputResource].Resource->Map(
					0,
					&ReadRange,
					reinterpret_cast<void**>(&RenderPass.MappedInstanceBuffers[OutputResource]))))
			{
				assert(false);
				return false;
			}
			if (!HDirectX::CreateOrUpdateStructuredBufferSRV(
					RenderPass.InstanceBufferDescriptors[OutputResource],
					RenderPass.InstanceBufferResources[OutputResource].Resource,
					0,
					1,
					sizeof(glm::vec4),
					RenderPass.CBVSRVUAVDescriptorHeap,
					DirectXContext.Device))
			{
				assert(false);
				return false;
			}
		}
	}

	RenderPass.SceneBufferIndex =
		RenderPass.RootSignature
			.AddRootParameter("SceneBuffer", HRootParameterType::CBV, 1, 0, HShaderVisibility::Vertex);
	RenderPass.TextureArrayRootParameter = RenderPass.RootSignature.AddRootParameter(
		"TextureArray",
		HRootParameterType::SRV,
		HRenderPass::MaxTextureCount,
		1,
		HShaderVisibility::Pixel);
	RenderPass.InstanceBufferIndex =
		RenderPass.RootSignature
			.AddRootParameter("InstanceBuffer", HRootParameterType::SRV, 1, 0, HShaderVisibility::Vertex);
	RenderPass.RootSignature.Build(DirectXContext, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> VSBlob;
	if (!HDirectX::CompileShader("Shaders/Hive/HRenderPass.hlsl", "VSMain", "vs_5_1", VSBlob))
	{
		assert(false);
		return false;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> PSBlob;
	if (!HDirectX::CompileShader("Shaders/Hive/HRenderPass.hlsl", "PSMain", "ps_5_1", PSBlob))
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
	for (uint64_t OutputResource = 0; OutputResource < HRenderPass::OutputBufferCount; OutputResource++)
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
			IID_PPV_ARGS(&RenderPass.VertexBufferResources[OutputResource]));
		if (!SUCCEEDED(Result))
		{
			assert(false);
			return false;
		}

		// Map the GPU buffer so we can write to it
		CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
		Result = RenderPass.VertexBufferResources[OutputResource]->Map(
			0,
			&ReadRange,
			reinterpret_cast<void**>(&RenderPass.MappedVertexBufferData[OutputResource]));
		if (!SUCCEEDED(Result))
		{
			assert(false);
			return false;
		}
		memcpy(RenderPass.MappedVertexBufferData[OutputResource], TriangleVertices, sizeof(TriangleVertices));

		// Initialize the vertex buffer view.
		RenderPass.VertexBufferViews[OutputResource].BufferLocation =
			RenderPass.VertexBufferResources[OutputResource]->GetGPUVirtualAddress();
		RenderPass.VertexBufferViews[OutputResource].StrideInBytes = sizeof(HVertex);
		RenderPass.VertexBufferViews[OutputResource].SizeInBytes = VertexBufferSize;
	}

	// Allocate Range of Descriptors for the TextureArray
	{
		for (uint64_t TextureArrayIndex = 0; TextureArrayIndex < HRenderPass::MaxTextureCount; TextureArrayIndex++)
		{
			RenderPass.TextureDescriptors[TextureArrayIndex] = RenderPass.CBVSRVUAVDescriptorHeap.AllocateDescriptor();
		}
	}

	return true;
}

bool HHoney::RenderRenderPass(
	HDirectXContext& DirectXContext,
	HRenderPass& RenderPass,
	const glm::vec3& Translation,
	const glm::vec3& Scale,
	const HScene& Scene,
	const std::vector<HInstancedMesh>& InstanedMeshes,
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
		for (const HInstancedMesh& InstanedMesh : InstanedMeshes)
		{
			VertexBufferSize += sizeof(HVertex) * InstanedMesh.Mesh->Verticies.size();

			if (InstanedMesh.TextureEntity != entt::null)
			{
				// Allocate Texture Descriptor
				{
					const auto& FoundTexture = RenderPass.TextureIndexMap.find(InstanedMesh.TextureEntity);
					if (FoundTexture == RenderPass.TextureIndexMap.end())
					{
						uint64_t TextureDescriptorIndex = RenderPass.TextureCount++;
						RenderPass.TextureIndexMap.insert({ InstanedMesh.TextureEntity, TextureDescriptorIndex });
					}
				}

				// Update Texture Descriptors
				const auto& FoundTexture = RenderPass.TextureIndexMap.find(InstanedMesh.TextureEntity);
				if (FoundTexture != RenderPass.TextureIndexMap.end())
				{
					HDescriptor& TextureDescriptor = RenderPass.TextureDescriptors[FoundTexture->second];
					const HTexture& Texture = Scene.Registry.get<HTexture>(InstanedMesh.TextureEntity);
					HDirectX::CreateOrUpdateSRV(
						TextureDescriptor,
						Texture.Resource.Resource,
						RenderPass.CBVSRVUAVDescriptorHeap,
						DirectXContext.Device);
				}
			}
		}
		if (VertexBufferSize > 0)
		{
			// Resize the vertex buffer
			if (RenderPass.VertexBufferViews[BackBufferIndex].SizeInBytes != VertexBufferSize)
			{
				OutputDebugStringA(std::format("Trigger VertexBuffer[{}] Resize.\n", BackBufferIndex).c_str());

				RenderPass.VertexBufferResources[BackBufferIndex]->Unmap(0, nullptr);
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
						IID_PPV_ARGS(&RenderPass.VertexBufferResources[BackBufferIndex]))))
				{
					return false;
				}
				OutputDebugStringA(
					std::format("Resized VertexBuffer[{}] to {}.\n", BackBufferIndex, VertexBufferSize).c_str());

				// Map the GPU buffer so we can write to it
				CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
				if (!CheckResult(RenderPass.VertexBufferResources[BackBufferIndex]->Map(
						0,
						&ReadRange,
						reinterpret_cast<void**>(&RenderPass.MappedVertexBufferData[BackBufferIndex]))))
				{
					assert(false);
					return false;
				}
				OutputDebugStringA(std::format("Mapped VertexBuffer[{}].\n", BackBufferIndex).c_str());

				// Initialize the vertex buffer view.
				RenderPass.VertexBufferViews[BackBufferIndex].BufferLocation =
					RenderPass.VertexBufferResources[BackBufferIndex]->GetGPUVirtualAddress();
				RenderPass.VertexBufferViews[BackBufferIndex].StrideInBytes = sizeof(HVertex);
				RenderPass.VertexBufferViews[BackBufferIndex].SizeInBytes = VertexBufferSize;
			}
		}
		// Update the Mesh Data
		{
			uint64_t VertexBufferIndex = 0;
			for (const HInstancedMesh& InstanedMesh : InstanedMeshes)
			{
				uint64_t MeshVertexBufferSize = sizeof(HVertex) * InstanedMesh.Mesh->Verticies.size();
				assert(VertexBufferIndex * sizeof(HVertex) + MeshVertexBufferSize <= VertexBufferSize);
				memcpy(
					RenderPass.MappedVertexBufferData[BackBufferIndex] + VertexBufferIndex,
					InstanedMesh.Mesh->Verticies.data(),
					MeshVertexBufferSize);
				VertexBufferIndex += InstanedMesh.Mesh->Verticies.size();
			}
		}

		// Resize the instance Buffers
		{
			uint64_t InstanceBufferSize = 0;
			for (const HInstancedMesh& InstanedMesh : InstanedMeshes)
			{
				InstanceBufferSize += sizeof(HInstanceBuffer) * InstanedMesh.Translations.size();
			}

			uint64_t OldInstanceBufferSize =
				RenderPass.InstanceBufferResources[BackBufferIndex].Resource->GetDesc().Width;
			if (InstanceBufferSize > OldInstanceBufferSize)
			{
				RenderPass.InstanceBufferResources[BackBufferIndex].Resource->Unmap(0, nullptr);
				RenderPass.MappedInstanceBuffers[BackBufferIndex] = nullptr;

				// Create Resource
				if (!HDirectX::CreateOrUpdateUploadBufferResource(
						RenderPass.InstanceBufferResources[BackBufferIndex],
						DirectXContext.Device,
						InstanceBufferSize))
				{
					return false;
				}

				// Map Resource
				CD3DX12_RANGE ReadRange(0, 0); // We do not intend to read from this resource on the CPU.
				if (!CheckResult(RenderPass.InstanceBufferResources[BackBufferIndex].Resource->Map(
						0,
						&ReadRange,
						reinterpret_cast<void**>(&RenderPass.MappedInstanceBuffers[BackBufferIndex]))))
				{
					return false;
				}

				// Create SRV
				if (!HDirectX::CreateOrUpdateStructuredBufferSRV(
						RenderPass.InstanceBufferDescriptors[BackBufferIndex],
						RenderPass.InstanceBufferResources[BackBufferIndex].Resource,
						0,
						InstanceBufferSize / sizeof(HInstanceBuffer),
						sizeof(HInstanceBuffer),
						RenderPass.CBVSRVUAVDescriptorHeap,
						DirectXContext.Device))
				{
					assert(false);
					return false;
				}
			}
		}

		// Update the instance buffers
		{
			uint64_t InstanceBufferIndex = 0;

			for (const HInstancedMesh& InstanedMesh : InstanedMeshes)
			{
				uint64_t MeshInstanceBufferSize = sizeof(HInstanceBuffer) * InstanedMesh.Translations.size();
				memcpy(
					RenderPass.MappedInstanceBuffers[BackBufferIndex] + InstanceBufferIndex,
					InstanedMesh.Translations.data(),
					MeshInstanceBufferSize);

				const auto& FoundTexture = RenderPass.TextureIndexMap.find(InstanedMesh.TextureEntity);
				if (FoundTexture != RenderPass.TextureIndexMap.end())
				{
					uint64_t TextureIndex = FoundTexture->second;
					uint64_t EndInstanceIndex = InstanceBufferIndex + InstanedMesh.Translations.size();
					for (uint64_t InstanceIndex = InstanceBufferIndex; InstanceIndex < EndInstanceIndex;
						 InstanceIndex++)
					{
						RenderPass.MappedInstanceBuffers[BackBufferIndex][InstanceIndex].Translation.w =
							*reinterpret_cast<float*>(&TextureIndex);
					}
				}
				else
				{
					assert(false);
				}
				InstanceBufferIndex += InstanedMesh.Translations.size();
			}
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

	ID3D12DescriptorHeap* Heaps[] = { RenderPass.CBVSRVUAVDescriptorHeap.DescriptorHeap };
	RenderPass.CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

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

	// Draw Meshes
	if (!InstanedMeshes.empty())
	{
		// Set necessary state.
		RenderPass.CommandList->SetGraphicsRootSignature(RenderPass.RootSignature.RootSiganature.Get());

		RenderPass.CommandList->SetGraphicsRootDescriptorTable(
			static_cast<uint32_t>(RenderPass.SceneBufferIndex),
			RenderPass.SceneBufferDescriptors[BackBufferIndex].GPUDescriptorHandle);
		RenderPass.CommandList->SetGraphicsRootDescriptorTable(
			static_cast<uint32_t>(RenderPass.TextureArrayRootParameter),
			RenderPass.TextureDescriptors[0].GPUDescriptorHandle);

		RenderPass.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		RenderPass.CommandList->IASetVertexBuffers(0, 1, &RenderPass.VertexBufferViews[BackBufferIndex]);

		RenderPass.CommandList->SetGraphicsRootDescriptorTable(
			static_cast<uint32_t>(RenderPass.InstanceBufferIndex),
			RenderPass.InstanceBufferDescriptors[BackBufferIndex].GPUDescriptorHandle);

		uint64_t InstanceOffset = 0;
		for (const HInstancedMesh& InstanedMesh : InstanedMeshes)
		{
			// Draw the same mesh at multiple locations
			RenderPass.CommandList->DrawInstanced(
				glm::max(3ull, InstanedMesh.Mesh->Verticies.size()),
				InstanedMesh.Translations.size(),
				0,
				InstanceOffset);
			InstanceOffset += InstanedMesh.Translations.size();
		}
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

// D3D12 WARNING : Live ID3D12Device at 0x000001CEAE692B48, Refcount : 45[STATE_CREATION WARNING #274: LIVE_DEVICE]
// D3D12 WARNING : Live            ID3D12RootSignature : 8[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live            ID3D12PipelineState : 11[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live                 ID3D12Resource : 81[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live                     ID3D12Heap : 60[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live             ID3D12CommandQueue : 3[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live                    ID3D12Fence : 15[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live         ID3D12CommandAllocator : 51[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live      ID3D12GraphicsCommandList : 9[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
// D3D12 WARNING : Live           ID3D12DescriptorHeap : 2[STATE_CREATION WARNING #255: LIVE_OBJECT_SUMMARY]
void HHoney::DestroyRenderPass(HRenderPass& RenderPass)
{
	if (RenderPass.FenceValue > 0)
	{
		HDirectX::WaitForFence(RenderPass.Fence, RenderPass.FenceValue);
	}
	for (uint64_t OutputResource = 0; OutputResource < HRenderPass::OutputBufferCount; OutputResource++)
	{
		if (RenderPass.VertexBufferResources[OutputResource])
		{
			RenderPass.MappedVertexBufferData[OutputResource] = nullptr;
			RenderPass.VertexBufferResources[OutputResource]->Unmap(0, nullptr);
		}

		RenderPass.MappedInstanceBuffers[OutputResource] = nullptr;
		RenderPass.InstanceBufferResources[OutputResource].Release();

		RenderPass.MappedSceneBuffers[OutputResource] = nullptr;
		RenderPass.SceneBufferResources[OutputResource].Release();

		RenderPass.OutputResources[OutputResource].Release();

		RenderPass.PipelineState.Reset();

		RenderPass.RootSignature.Release();

		RenderPass.TextureIndexMap.clear();

		RenderPass.RTVDescriptorHeap.Release();
		RenderPass.CBVSRVUAVDescriptorHeap.Release();

		RenderPass.Fence.Release();

		RenderPass.UpdateCommandList->Release();
		RenderPass.UpdateCommandAllocator->Release();

		RenderPass.CommandList->Release();
		RenderPass.CommandAllocator->Release();
	}
}
