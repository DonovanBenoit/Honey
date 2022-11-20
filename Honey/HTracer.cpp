#include "HTracer.h"

#include "HLessons.h"

#include <imgui.h>

namespace
{
	HCamera Camera{};

	void DrawTracer()
	{
		glm::vec2 ContentRegion = ImGui::GetContentRegionAvail();

		const float ControlsWidth = ContentRegion.x * 0.25f;
		const float ViewWidth = ContentRegion.x - ControlsWidth;

		if (ImGui::BeginChild("Controls", { ControlsWidth, ContentRegion.y }))
		{
			ImGui::SliderFloat("Focal Length", &Camera.FocalLength, 0.01f, 4.0f);
			ImGui::SliderFloat3("Position", &Camera.Translation.x, -10.0f, 10.0f);

			float PannelHeight = ImGui::GetStyle().FramePadding.y * 2.0f + ImGui::GetFrameHeightWithSpacing() * 4.0f + ImGui::GetTextLineHeightWithSpacing();
			if (ImGui::BeginChild("SubPannel", { 0.0f, PannelHeight }))
			{
				ImGui::Text("Text");
				ImGui::Button("A");
				ImGui::Button("B");
				ImGui::Button("C");
				ImGui::Button("D");
				ImGui::Button("E");
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("View", { ViewWidth, ContentRegion.y }))
		{
			ImVec2 CursorPosition = ImGui::GetCursorScreenPos();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();

			ImVec2 CameraPosition = { CursorPosition.x + Camera.Translation.x * 100.0f,
									  CursorPosition.y + Camera.Translation.z * 100.0f };

			int32_t Y = 0;
			for (int32_t X = 0; X < Camera.Width; X++)
			{
				glm::vec3 Direction = { (static_cast<float>(X) + 0.5f) / static_cast<float>(Camera.Width) - 0.5f,
										(static_cast<float>(Y) + 0.5f) / static_cast<float>(Camera.Height) - 0.5f,
										Camera.FocalLength };
				Direction = glm::normalize(Direction);

				DrawList->AddLine(
					CameraPosition,
					{ CameraPosition.x + Direction.x * 1000.0f, CameraPosition.y + Direction.z * 1000.0f },
					0XFFFF4444);
			}
		}
		ImGui::EndChild();
	}
} // namespace

void HHoney::DrawHoney(const glm::vec2& Resolution)
{
	ImGui::SetNextWindowPos({});
	ImGui::SetNextWindowSize(Resolution);
	if (ImGui::Begin("MainWindow"))
	{
		if (ImGui::BeginTabBar("MainTabBar"))
		{
			if (ImGui::BeginTabItem("Tracer"))
			{
				DrawTracer();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lessons"))
			{
				DrawLessons();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}
