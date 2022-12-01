#include "HLessons.h"

#include "HScene.h"

#include <HImGui.h>

#include <glm/gtx/rotate_vector.hpp>

namespace
{
	using HViewFunctionPtr = void (*)(const glm::vec3& RightDirection, const glm::vec3& DownDirection);

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
}