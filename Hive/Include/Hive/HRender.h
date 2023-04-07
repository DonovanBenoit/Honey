#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

struct HGUIImage;
struct HGUIWindow;
struct HScene;

struct HRenderWindow
{
	int64_t ImageIndex = -1;
};

namespace HHoney
{
	void Render(HScene& Scene, HGUIImage& Image, entt::entity CameraEntity);

	void DrawRender(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow);
}