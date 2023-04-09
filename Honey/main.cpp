#include "HImGui.h"
#include <Hive/HRender.h>
#include <Hive/HScene.h>
#include "HTracer.h"
#include "HWindow.h"

#include <HDirectX.h>
#include <Windows.h>

// Main code
int main(int, char**)
{
	HGUIWindow GUIWindow{};
	HImGui::CreateGUIWindow(GUIWindow);

	// Our state
	HScene Scene{};
	HHoney::DefaultScene(Scene);
	HRenderWindow RenderWindow{};

	ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool Quit = false;
	while (!Quit)
	{
		HImGui::NewFrame(GUIWindow, Quit);
		if (Quit)
		{
			break;
		}

		DXGI_SWAP_CHAIN_DESC SwapChainDeesc;
		GUIWindow.SwapChain.SwapChain->GetDesc(&SwapChainDeesc);
		HHoney::DrawHoney({ SwapChainDeesc.BufferDesc.Width, SwapChainDeesc.BufferDesc.Height }, GUIWindow, Scene, RenderWindow);

		HHoney::UpdateScene(Scene, Scene.Cameras[0]);

		// Rendering
		ImGui::Render();

		HFrameContext* frameCtx = HImGui::WaitForNextFrameResources(GUIWindow);
		UINT backBufferIdx = GUIWindow.SwapChain.SwapChain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GUIWindow.RenderTargetResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		GUIWindow.DirectXContext->CommandList->Reset(frameCtx->CommandAllocator, NULL);
		GUIWindow.DirectXContext->CommandList->ResourceBarrier(1, &barrier);

		// Render Dear ImGui graphics
		GUIWindow.DirectXContext->CommandList->ClearRenderTargetView(
			GUIWindow.RenderTargetDescriptor[backBufferIdx],
			reinterpret_cast<float*>(&ClearColor),
			0,
			NULL);
		GUIWindow.DirectXContext->CommandList
			->OMSetRenderTargets(1, &GUIWindow.RenderTargetDescriptor[backBufferIdx], FALSE, NULL);
		GUIWindow.DirectXContext->CommandList->SetDescriptorHeaps(1, &GUIWindow.CBVSRVUAV_DescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUIWindow.DirectXContext->CommandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		GUIWindow.DirectXContext->CommandList->ResourceBarrier(1, &barrier);
		GUIWindow.DirectXContext->CommandList->Close();

		GUIWindow.DirectXContext->CommandQueue->ExecuteCommandLists(
			1,
			(ID3D12CommandList* const*)&GUIWindow.DirectXContext->CommandList);

		GUIWindow.SwapChain.SwapChain->Present(1, 0); // Present with vsync

		if (!HDirectX::SignalFence(
				GUIWindow.DirectXContext->CommandQueue,
				GUIWindow.DirectXContext->Fence,
				frameCtx->FenceValue))
		{
			break;
		}
	}

	HImGui::WaitForLastSubmittedFrame();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	HImGui::DestroyGUIWindow(GUIWindow);

	return 0;
}