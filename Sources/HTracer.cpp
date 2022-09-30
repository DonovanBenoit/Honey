#include "HTracer.h"

#include <imgui.h>

namespace
{
	HCamera Camera{};
}

void HTracer::DrawTracer(const glm::vec2& Resolution)
{
	const float ControlsWidth = 250.0f;
	const float ViewWidth = Resolution.x - ControlsWidth;

	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize({ ControlsWidth, Resolution.y });
	if (ImGui::Begin("Controls"))
	{
		ImGui::SliderFloat("Focal Length", &Camera.FocalLength, 0.01f, 4.0f);
		ImGui::SliderFloat3("Position", &Camera.Translation.x, -10.0f, 10.0f);
	}
	ImGui::End();

	ImGui::SetNextWindowPos({ ControlsWidth, 0.0f });
	ImGui::SetNextWindowSize({ ViewWidth, Resolution.y });
	if (ImGui::Begin("View"))
	{
			ImVec2 CursorPosition = ImGui::GetCursorScreenPos();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			// DrawList->AddRect(CursorPosition, { CursorPosition.x + 100, CursorPosition.y + 100 }, 0xFF00FFFF);

			ImVec2 CameraPosition = { CursorPosition.x + Camera.Translation.x * 100.0f, CursorPosition.y + Camera.Translation.z * 100.0f };

			int32_t Y = 0;
			for (int32_t X = 0; X < Camera.Width; X++)
			{
				glm::vec3 Direction = { (static_cast<float>(X) + 0.5f) / static_cast<float>(Camera.Width) - 0.5f, (static_cast<float>(Y) + 0.5f) / static_cast<float>(Camera.Height) - 0.5f, Camera.FocalLength };
				Direction = glm::normalize(Direction);

				DrawList->AddLine(CameraPosition, { CameraPosition.x + Direction.x * 1000.0f, CameraPosition.y + Direction.z * 1000.0f }, 0XFFFF4444);
			}	
	}
	ImGui::End();
}