#include "HTracer.h"

#include "HScene.h"
#include "HWindow.h"

#include <HImGui.h>
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
					if (ImGui::TreeNodeEx(std::format("Camera_{}", (uint32_t)CameraEntity).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
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
					if (ImGui::TreeNodeEx(std::format("Sphere_{}", (uint32_t)SphereEntity).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
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
					if (ImGui::TreeNodeEx(std::format("PointLight_{}", (uint32_t)PointLightEntity).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						HPointLight& PointLight = Scene.Get<HPointLight>(PointLightEntity);
						HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(PointLightEntity);
						ImGui::ColorEdit3("Focal Length", &PointLight.Color.r);
						ImGui::SliderDouble3("Translation", &RelativeTransform.Translation.x, -10.0, 10.0);
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::EndChild();
	}

	bool TraceRay(
		HScene& Scene,
		const glm::vec3& RayOrigin,
		const glm::vec3& RayDirection,
		glm::vec3& HitPosition,
		glm::vec3& HitNormal)
	{
		float FarClip = 10000000.0f;
		float THit = FarClip;

		auto Spheres = Scene.Registry.view<HSphere>();
		for (entt::entity SphereEntity : Spheres)
		{
			HSphere& Sphere = Scene.Get<HSphere>(SphereEntity);
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(SphereEntity);

			float SphereHit;
			if (glm::intersectRaySphere(RayOrigin, RayDirection, WorldTransform.Translation, Sphere.Radius, SphereHit))
			{
				if (SphereHit < THit)
				{
					THit = SphereHit;

					HitPosition = RayOrigin + RayDirection * THit;
					HitNormal = (HitPosition - WorldTransform.Translation) / Sphere.Radius;
				}
			}
		}

		return THit < FarClip;
	}

	glm::vec3 Lighting(HScene& Scene, const glm::vec3& WorldPosition, const glm::vec3& WorldNormal)
	{
		glm::vec3 Color = {};

		auto PointLightView = Scene.Registry.view<HPointLight>();
		for (entt::entity PointLightEntity : PointLightView)
		{
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(PointLightEntity);
			HPointLight& PointLight = Scene.Get<HPointLight>(PointLightEntity);

			const glm::vec3 Displacement = WorldTransform.Translation - WorldPosition;
			float Distance = glm::length(Displacement);
			if (Distance < PointLight.Radius)
			{
				Color += PointLight.Color;
			}
			else
			{
				float LightDot = glm::dot(Displacement / Distance, WorldNormal);

				Color += PointLight.Color * glm::max(LightDot, 0.0f) / (Distance * Distance);
			}
		}

		return Color;
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

		const float AspectRatio = float(Image.Width) / float(Image.Height);

		for (uint32_t Y = 0; Y < Image.Height; Y++)
		{
			for (uint32_t X = 0; X < Image.Width; X++)
			{
				entt::entity CameraEntity = Scene.Cameras[0];
				HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
				HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

				glm::vec3 RayDirection = { (float(X) + 0.5f - float(Image.Width) / 2.0f) / float(Image.Width),
										   (float(Y) + 0.5f - float(Image.Height) / 2.0f) / float(Image.Height)
											   / AspectRatio,
										   Camera.FocalLength };
				RayDirection = glm::normalize(RayDirection);

				glm::vec3 IntersectionPoint = {};
				glm::vec3 IntersectionNormal = {};

				glm::vec3 Color;
				if (TraceRay(Scene, CameraTransform.Translation, RayDirection, IntersectionPoint, IntersectionNormal))
				{
					Color = Lighting(Scene, IntersectionPoint, IntersectionNormal) * glm::vec3(0.24f, 0.69f, 0.42f);
				}
				else
				{
					Color = { 0.36f, 0.69f, 0.96f };
				}

				uint64_t PixelOffset1D = Y * Image.Width + X;
				Image.Pixels[PixelOffset1D] = ImGui::ColorConvertFloat4ToU32(glm::vec4(Color, 1.0f));
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
