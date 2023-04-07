#include "HTracer.h"

#include "Hive/HRender.h"
#include "Hive/HScene.h"
#include "HWindow.h"

#include <HImGui.h>
#include <chrono>
#include <entt/entt.hpp>
#include <format>
#include <glm/gtx/intersect.hpp>
#include <imgui.h>

namespace
{

	void DrawControls(HScene& Scene)
	{
		if (ImGui::BeginChild("Controls"))
		{
			if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (entt::entity CameraEntity : Scene.Cameras)
				{
					if (ImGui::TreeNodeEx(
							std::format("Camera_{}", (uint32_t)CameraEntity).c_str(),
							ImGuiTreeNodeFlags_DefaultOpen))
					{
						HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
						HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(CameraEntity);
						ImGui::SliderFloat("Focal Length", &Camera.FocalLength, 0.01f, 4.0f);
						ImGui::SliderDouble3("Translation", &RelativeTransform.Translation.x, -10.0, 10.0);
						ImGui::TreePop();
					}
				}

				for (entt::entity SphereEntity : Scene.Spheres)
				{
					if (ImGui::TreeNodeEx(
							std::format("Sphere_{}", (uint32_t)SphereEntity).c_str(),
							ImGuiTreeNodeFlags_DefaultOpen))
					{
						HSphere& Sphere = Scene.Get<HSphere>(SphereEntity);
						HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(SphereEntity);
						ImGui::SliderFloat("Radius", &Sphere.Radius, 0.01f, 4.0f);
						ImGui::SliderDouble3("Translation", &RelativeTransform.Translation.x, -10.0, 10.0);
						ImGui::TreePop();
					}
				}

				for (entt::entity PointLightEntity : Scene.PointLights)
				{
					if (ImGui::TreeNodeEx(
							std::format("PointLight_{}", (uint32_t)PointLightEntity).c_str(),
							ImGuiTreeNodeFlags_DefaultOpen))
					{
						HPointLight& PointLight = Scene.Get<HPointLight>(PointLightEntity);
						HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(PointLightEntity);
						ImGui::ColorEdit3("Color", &PointLight.Color.r);
						ImGui::SliderFloat("Radius", &PointLight.Radius, 0.01f, 4.0f);
						ImGui::SliderDouble3("Translation", &RelativeTransform.Translation.x, -10.0, 10.0);
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::EndChild();
	}

} // namespace


void HHoney::DrawHoney(const glm::vec2& Resolution, HGUIWindow& GUIWindow, HScene& Scene, HRenderWindow& RenderWindow)
{
	ImGui::SetNextWindowPos({});
	ImGui::SetNextWindowSize(Resolution);

	const float Golden = (1.0f + glm::sqrt(5.0f)) / 2.0f;

	glm::vec2 TracerSize = glm::round(glm::vec2{ Resolution.x / Golden, Resolution.y });
	glm::vec2 ControlsSize = { Resolution.x - TracerSize.x, Resolution.y };

	ImGui::SetNextWindowSize(TracerSize);
	if (ImGui::Begin("TracerWindow"))
	{
		DrawRender(GUIWindow, Scene, Scene.Cameras[0], RenderWindow);
	}
	ImGui::End();

	ImGui::SetNextWindowPos(glm::vec2{ TracerSize.x, 0.0f });
	ImGui::SetNextWindowSize(ControlsSize);
	if (ImGui::Begin("ControlsWindow"))
	{
		DrawControls(Scene);
	}
	ImGui::End();
}
