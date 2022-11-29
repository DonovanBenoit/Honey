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
		GUIWindow.DirectXContext->Device,
		HDirectXContext::NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		GUIWindow.CBVSRVUAV_DescHeap,
		GUIWindow.CBVSRVUAV_DescHeap->GetCPUDescriptorHandleForHeapStart(),
		GUIWindow.CBVSRVUAV_DescHeap->GetGPUDescriptorHandleForHeapStart());

	// Our state
	ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	int64_t ImageIndex = -1;
	if (!HImGui::CreateImage(GUIWindow, ImageIndex, 512, 512))
	{
		return 1;
	}

	HGUIImage& Image = GUIWindow.Images[ImageIndex];
	for (uint64_t Y = 0; Y < Image.Height; Y++)
	{
		for (uint64_t X = 0; X < Image.Width; X++)
		{
			uint64_t PixelOffset1D = Y * Image.Width + X;
			Image.RGBA[PixelOffset1D * 4 + 0] = X % 256;
			Image.RGBA[PixelOffset1D * 4 + 1] = Y % 256;
			Image.RGBA[PixelOffset1D * 4 + 2] = 0;
			Image.RGBA[PixelOffset1D * 4 + 3] = 255;
		}
	}

	if (!HImGui::UploadImage(GUIWindow, ImageIndex, 512, 512))
	{
		return 1;
	}

	// Main loop
	bool Quit = false;
	while (!Quit)
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
				Quit = true;
		}
		if (Quit)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DXGI_SWAP_CHAIN_DESC SwapChainDeesc;
		GUIWindow.SwapChain.SwapChain->GetDesc(&SwapChainDeesc);
		HHoney::DrawHoney({ SwapChainDeesc.BufferDesc.Width, SwapChainDeesc.BufferDesc.Height });

		if (ImGui::Begin("Render"))
		{
			HImGui::DrawImage(GUIWindow, ImageIndex);
		}
		ImGui::End();

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
		GUIWindow.DirectXContext->CommandList
			->ClearRenderTargetView(GUIWindow.RenderTargetDescriptor[backBufferIdx], reinterpret_cast<float*>(&ClearColor), 0, NULL);
		GUIWindow.DirectXContext->CommandList->OMSetRenderTargets(1, &GUIWindow.RenderTargetDescriptor[backBufferIdx], FALSE, NULL);
		GUIWindow.DirectXContext->CommandList->SetDescriptorHeaps(1, &GUIWindow.CBVSRVUAV_DescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUIWindow.DirectXContext->CommandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		GUIWindow.DirectXContext->CommandList->ResourceBarrier(1, &barrier);
		GUIWindow.DirectXContext->CommandList->Close();

		GUIWindow.DirectXContext->CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&GUIWindow.DirectXContext->CommandList);

		GUIWindow.SwapChain.SwapChain->Present(1, 0); // Present with vsync

		if (!HDirectX::SignalFence(GUIWindow.DirectXContext->CommandQueue, GUIWindow.DirectXContext->Fence, frameCtx->FenceValue))
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