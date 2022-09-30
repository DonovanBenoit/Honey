#include "HLessons.h"

#include "HScene.h"

#include <HImGui.h>

#include <glm/gtx/rotate_vector.hpp>

namespace
{
	using HViewFunctionPtr = void (*)(const glm::vec3& RightDirection, const glm::vec3& DownDirection);
	static HDirectionalLight DirectionalLight{};

	glm::vec3 Forward{ 0.0f, 0.0f, 1.0f };
	glm::vec3 Right{ 1.0f, 0.0f, 0.0f };
	glm::vec3 Down{ 0.0f, -1.0f, 0.0f };

	void QuadView(HViewFunctionPtr ViewFunctionPtr)
	{
		glm::vec2 CursorPosition = ImGui::GetCursorScreenPos();
		glm::vec2 ContentRegion = ImGui::GetContentRegionAvail();
		float LeftWidth = ContentRegion.x / 2.0f;
		float RightWidth = ContentRegion.x - LeftWidth;
		float TopHeight = ContentRegion.y / 2.0f;
		float BottomHeight = ContentRegion.y - TopHeight;

		if (ImGui::BeginChild("Top", { LeftWidth, TopHeight }))
		{
			ViewFunctionPtr(Right, Forward);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("3D", { RightWidth, TopHeight }))
		{
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("Back", { LeftWidth, BottomHeight }))
		{
			ViewFunctionPtr(Right, Down);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("Right", { RightWidth, BottomHeight }))
		{
			ViewFunctionPtr(Forward, Down);
		}
		ImGui::EndChild();

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRect(CursorPosition, CursorPosition + glm::vec2{ LeftWidth, TopHeight }, 0xFFFFFFFF);
		DrawList->AddText(CursorPosition, 0xFFFFFFFF, "Top");

		DrawList->AddRect(CursorPosition + glm::vec2{ LeftWidth, 0.0f }, CursorPosition + glm::vec2{ LeftWidth + RightWidth, TopHeight }, 0xFFFFFFFF);
		DrawList->AddText(CursorPosition + glm::vec2{ LeftWidth, 0.0f }, 0xFFFFFFFF, "3D");

		DrawList->AddRect(CursorPosition + glm::vec2{ 0.0f, TopHeight }, CursorPosition + glm::vec2{ LeftWidth, TopHeight + BottomHeight }, 0xFFFFFFFF);
		DrawList->AddText(CursorPosition + glm::vec2{ 0.0f, TopHeight }, 0xFFFFFFFF, "Back");

		DrawList->AddRect(CursorPosition + glm::vec2{ LeftWidth, TopHeight }, CursorPosition + glm::vec2{ LeftWidth + RightWidth, TopHeight + BottomHeight }, 0xFFFFFFFF);
		DrawList->AddText(CursorPosition + glm::vec2{ LeftWidth, TopHeight }, 0xFFFFFFFF, "Right");
	};

	void DirectionalLightView(const glm::vec3& RightDirection, const glm::vec3& DownDirection)
	{
		glm::vec2 ContentRegion = ImGui::GetContentRegionAvail();

		glm::vec2 CursorPosition = ImGui::GetCursorScreenPos();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		glm::vec2 LightDirection{ glm::dot(RightDirection, DirectionalLight.Direction),
								  glm::dot(DownDirection, DirectionalLight.Direction) };

		glm::vec2 LightPlane = glm::rotate(LightDirection, glm::half_pi<float>());

		float RayCount = 32;
		float RayPitch = 24.0f;
		for (float Ray = 0; Ray < RayCount; Ray += 1.0f)
		{
			glm::vec2 RayStart = CursorPosition + LightPlane * (Ray * RayPitch);
			glm::vec2 RayEnd = RayStart + LightDirection * 1000.0f;

			DrawList->AddLine(RayStart, RayEnd, 0XFFFF4444);
		}
	}

	void DirectionalLightLesson()
	{

		glm::vec2 ContentRegion = ImGui::GetContentRegionAvail();

		const float ControlsWidth = ContentRegion.x * 0.25f;
		const float ViewWidth = ContentRegion.x - ControlsWidth;

		if (ImGui::BeginChild("Controls", { ControlsWidth, ContentRegion.y }))
		{
			ImGui::SliderFloat3("Color", &DirectionalLight.Intensity.x, 0.01f, 4.0f);
			if (ImGui::SliderFloat3("Direction", &DirectionalLight.Direction.x, -1.0f, 1.0f))
			{
				float MagnitudeSquared = glm::dot(DirectionalLight.Direction, DirectionalLight.Direction);
				if (MagnitudeSquared < 0.0001f)
				{
					DirectionalLight.Direction = glm::vec3(0.0f, 1.0f, 0.0f);
				}
				else
				{
					DirectionalLight.Direction /= glm::sqrt(MagnitudeSquared);
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("View", { ViewWidth, ContentRegion.y }))
		{
			QuadView(DirectionalLightView);
		}
		ImGui::EndChild();
	}
} // namespace

void HHoney::DrawLessons()
{
	if (ImGui::BeginTabBar("Lessons_Tabs"))
	{
		if (ImGui::BeginTabItem("Directional Light"))
		{
			DirectionalLightLesson();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}