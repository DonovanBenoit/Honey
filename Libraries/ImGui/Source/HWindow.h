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
#endif // _WIN32
};

namespace HImGui
{
	bool CreateGUIWindow(HGUIWindow& GUIWindow);
	void DestroyGUIWindow(HGUIWindow& GUIWindow);

	HFrameContext* WaitForNextFrameResources(HGUIWindow& GUIWindow);
	void WaitForLastSubmittedFrame();
}


