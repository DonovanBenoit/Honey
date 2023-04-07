#pragma once

#include <glm/glm.hpp>

struct HGUIImage;
struct HGUIWindow;
struct HRenderWindow;
struct HScene;

namespace HHoney
{
	void DrawHoney(const glm::vec2& Resolution, HGUIWindow& GUIWindow, HScene& Scene, HRenderWindow& Tracer);
}