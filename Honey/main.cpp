#include "HTracer.h"

#include "HImGui.h"
#include "HWindow.h"

#include <HDirectX.h>

#include <Windows.h>


// Main code
int main(int, char**)
{
	HGUIWindow GUIWindow{};

	HImGui::CreateGUIWindow(GUIWindow);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(GUIWindow.WindowHandle);
	ImGui_ImplDX12_Init(
		GUIWindow.DirectXContext->g_pd3dDevice,
		HDirectXContext::NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		GUIWindow.DirectXContext->g_pd3dSrvDescHeap,
		GUIWindow.DirectXContext->g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		GUIWindow.DirectXContext->g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	// Our state
	ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32
		// backend.
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DXGI_SWAP_CHAIN_DESC SwapChainDeesc;
		GUIWindow.DirectXContext->SwapChain.SwapChain->GetDesc(&SwapChainDeesc);
		HHoney::DrawHoney({ SwapChainDeesc.BufferDesc.Width, SwapChainDeesc.BufferDesc.Height });

		// Rendering
		ImGui::Render();

		HFrameContext* frameCtx = HImGui::WaitForNextFrameResources(GUIWindow);
		UINT backBufferIdx = GUIWindow.DirectXContext->SwapChain.SwapChain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GUIWindow.DirectXContext->g_mainRenderTargetResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		GUIWindow.DirectXContext->g_pd3dCommandList->Reset(frameCtx->CommandAllocator, NULL);
		GUIWindow.DirectXContext->g_pd3dCommandList->ResourceBarrier(1, &barrier);

		// Render Dear ImGui graphics
		GUIWindow.DirectXContext->g_pd3dCommandList
			->ClearRenderTargetView(GUIWindow.DirectXContext->g_mainRenderTargetDescriptor[backBufferIdx], reinterpret_cast<float*>(&ClearColor), 0, NULL);
		GUIWindow.DirectXContext->g_pd3dCommandList->OMSetRenderTargets(1, &GUIWindow.DirectXContext->g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
		GUIWindow.DirectXContext->g_pd3dCommandList->SetDescriptorHeaps(1, &GUIWindow.DirectXContext->g_pd3dSrvDescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUIWindow.DirectXContext->g_pd3dCommandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		GUIWindow.DirectXContext->g_pd3dCommandList->ResourceBarrier(1, &barrier);
		GUIWindow.DirectXContext->g_pd3dCommandList->Close();

		GUIWindow.DirectXContext->g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&GUIWindow.DirectXContext->g_pd3dCommandList);

		GUIWindow.DirectXContext->SwapChain.SwapChain->Present(1, 0); // Present with vsync

		UINT64 fenceValue = GUIWindow.DirectXContext->g_fenceLastSignaledValue + 1;
		GUIWindow.DirectXContext->g_pd3dCommandQueue->Signal(GUIWindow.DirectXContext->Fence.Fence, fenceValue);
		GUIWindow.DirectXContext->g_fenceLastSignaledValue = fenceValue;
		frameCtx->FenceValue = fenceValue;
	}

	HImGui::WaitForLastSubmittedFrame();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	HImGui::DestroyGUIWindow(GUIWindow);

	return 0;
}