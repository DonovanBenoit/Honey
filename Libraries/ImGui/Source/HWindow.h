// Copyright 2019 - DonovanBenoit
#pragma once

#ifdef _WIN32
	#include <HDirectX.h>
#endif // _WIN32

struct HGUIWindow
{
#ifdef _WIN32
	WNDCLASSEX WindowClass;
	HWND WindowHandle;
	HDirectXContext* DirectXContext = nullptr;

	static int32_t const NUM_BACK_BUFFERS = 3;
	ID3D12Resource* RenderTargetResource[NUM_BACK_BUFFERS] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
	HSwapChain SwapChain{};
#endif // _WIN32
};

namespace HImGui
{
	bool CreateGUIWindow(HGUIWindow* GUIWindow);
	void DestroyGUIWindow(HGUIWindow& GUIWindow);

	void CreateRenderTargets(HGUIWindow& GUIWindow);
	void DestroyRenderTargets(HGUIWindow& GUIWindow);

	HFrameContext* WaitForNextFrameResources(HGUIWindow& GUIWindow);
	void WaitForLastSubmittedFrame();
}


