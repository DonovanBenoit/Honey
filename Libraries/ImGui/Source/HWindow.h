// Copyright 2019 - DonovanBenoit
#pragma once

#ifdef _WIN32
	#include <HDirectX.h>
#endif // _WIN32

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include <Windows.h>

struct GLFWwindow;

struct HGUIImage
{
	uint64_t Width = 0;
	uint64_t Height = 0;

	union
	{
		uint8_t* Data = nullptr;
		uint8_t* Bytes;
		uint32_t* Pixels;
	};

	ID3D12Resource* Resource = nullptr;
	ID3D12Resource* UploadResource = nullptr;
	
	union
	{
		void* UploadData = nullptr;
		uint8_t* UploadBytes;
	};
	HDescriptor Descriptor{};
};

struct HGUIWindow
{
	GLFWwindow* Window = nullptr;
#ifdef _WIN32
	HWND WindowHandle;
	HDirectXContext* DirectXContext = nullptr;

	ID3D12DescriptorHeap* RTV_DescHeap = nullptr;
	HDescriptorHeap CBVSRVUAV_DescHeap{};

	HDescriptor ImGuiDescriptor{};

	static int32_t const NUM_BACK_BUFFERS = 3;
	ID3D12Resource* RenderTargetResource[NUM_BACK_BUFFERS] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
	HSwapChain SwapChain{};
#endif // _WIN32

	int64_t ImageCount = 0;
	std::array<HGUIImage, 1024 - 1> Images = {};
	std::vector<size_t> ImageRecycling{};
};

namespace HImGui
{
	bool CreateGUIWindow(HGUIWindow& GUIWindow);
	void DestroyGUIWindow(HGUIWindow& GUIWindow);
	void NewFrame(HGUIWindow& GUIWindow, bool& Quit);
	bool Render(HGUIWindow& GUIWindow);

	void CreateRenderTargets(HGUIWindow& GUIWindow);
	void DestroyRenderTargets(HGUIWindow& GUIWindow);

	glm::vec2 GetWindowSize(HGUIWindow& GUIWindow);

	HFrameContext* WaitForNextFrameResources(HGUIWindow& GUIWindow);
	void WaitForLastSubmittedFrame();

	bool CreateOrUpdateImage(HGUIWindow& GUIWindow, int64_t& ImageIndex, uint64_t Width, uint64_t Height);
	bool UploadImage(HGUIWindow& GUIWindow, int64_t ImageIndex);
	void DrawImage(HGUIWindow& GUIWindow, int64_t ImageIndex);
	bool DestroyImage(HGUIWindow& GUIWindow, int64_t ImageIndex);
}


