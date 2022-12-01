#include "HTracer.h"

#include "HLessons.h"
#include "HWindow.h"

#include <imgui.h>

#include <glm/gtx/intersect.hpp>

namespace
{
	void DrawControls(HScene& Scene)
	{
		glm::vec2 ContentRegion = ImGui::GetContentRegionAvail();

		const float ControlsWidth = ContentRegion.x * 0.25f;
		const float ViewWidth = ContentRegion.x - ControlsWidth;

		if (ImGui::BeginChild("Controls", { ControlsWidth, ContentRegion.y }))
		{
			ImGui::SliderFloat("Focal Length", &Scene.Camera.FocalLength, 0.01f, 4.0f);
			ImGui::SliderFloat3("Position", &Scene.Camera.Translation.x, -10.0f, 10.0f);
		}
		ImGui::EndChild();
	}

	void DrawTracer(HGUIWindow& GUIWindow, HScene& Scene, HTracer& Tracer)
	{
		ImVec2 Resolution = ImGui::GetContentRegionAvail();
		if (!HImGui::CreateOrUpdateImage(
				GUIWindow,
				Tracer.ImageIndex,
				static_cast<uint64_t>(Resolution.x),
				static_cast<uint64_t>(Resolution.y)))
		{
			return;
		}

		HGUIImage& Image = GUIWindow.Images[Tracer.ImageIndex];

		for (uint32_t Y = 0; Y < Image.Height; Y++)
		{
			for (uint32_t X = 0; X < Image.Width; X++)
			{
				glm::vec3 RayDirection = { (float(X) + 0.5f - float(Image.Width) / 2.0f) / float(Image.Width),
										   (float(Y) + 0.5f - float(Image.Height) / 2.0f) / float(Image.Height),
										   1.0f };
				RayDirection = glm::normalize(RayDirection);

				glm::vec3 IntersectionPoint = {};
				glm::vec3 IntersectionNormal = {};

				glm::vec3 Color = { 0.36f, 0.69f, 0.96f };
				if (glm::intersectRaySphere(
					{},
					RayDirection,
					{ 0.0f, 0.0f, 5.0f },
					1.0f,
					IntersectionPoint,
					IntersectionNormal))
				{
					float LightDistance = glm::length(IntersectionPoint - glm::vec3{ 1.0f, 0.0f, 3.0f });
					float LightDot = glm::dot(
						glm::normalize(glm::vec3{ 1.0f, 3.0f, 0.0f } - IntersectionPoint),
						IntersectionNormal);
					Color = glm::max(LightDot, 0.0f) * glm::max(1.0f / (LightDistance * LightDistance), 1.0f)
						* glm::vec3(0.24f, 0.69f, 0.42f);
				}

				uint64_t PixelOffset1D = Y * Image.Width + X;
				Image.Bytes[PixelOffset1D * 4 + 0] = glm::clamp<uint32_t>(Color.r * 256.0f, 0, 255);
				Image.Bytes[PixelOffset1D * 4 + 1] = glm::clamp<uint32_t>(Color.g * 256.0f, 0, 255);
				Image.Bytes[PixelOffset1D * 4 + 2] = glm::clamp<uint32_t>(Color.b * 256.0f, 0, 255);
				Image.Bytes[PixelOffset1D * 4 + 3] = 255;
			}
		}

		if (!HImGui::UploadImage(GUIWindow, Tracer.ImageIndex))
		{
			return;
		}

		HImGui::DrawImage(GUIWindow, Tracer.ImageIndex);
	}
} // namespace

void HHoney::DrawHoney(const glm::vec2& Resolution, HGUIWindow& GUIWindow, HScene& Scene, HTracer& Tracer)
{
	ImGui::SetNextWindowPos({});
	ImGui::SetNextWindowSize(Resolution);

	const float Golden = (1.0f + glm::sqrt(5.0f)) / 2.0f;

	glm::vec2 TracerSize = glm::round(glm::vec2{ Resolution.x / Golden, Resolution.y });
	glm::vec2 ControlsSize = { Resolution.x - TracerSize.x, Resolution.y };

	ImGui::SetNextWindowSize(TracerSize);
	if (ImGui::Begin("TracerWindow"))
	{
		DrawTracer(GUIWindow, Scene, Tracer);
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
