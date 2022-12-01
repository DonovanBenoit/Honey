#pragma once

#include "HImGui.h"
#include "HScene.h"

#include <glm/glm.hpp>

struct HGUIWindow;

struct HTracer
{
	int64_t ImageIndex = -1;
};

namespace HHoney
{
	void DrawHoney(const glm::vec2& Resolution, HGUIWindow& GUIWindow, HScene& Scene, HTracer& Tracer);
}