#include "HRender.h"

#include "HScene.h"
#include "HWindow.h"

#include <HImGui.h>

#include <chrono>
#include <thread>

#include <entt/entt.hpp>
#include <format>
#include <glm/gtx/intersect.hpp>
#include <imgui.h>

namespace
{
	bool RaySphereIntersect(HRenderedSphere& RenderedSphere, const glm::vec3& RayDirection, float& IntersectionDistance)
	{
		constexpr float Epsilon = std::numeric_limits<float>::epsilon();

		float t0 = glm::dot(RenderedSphere.RayOriginToSphereCenter, RayDirection);
		float d2 = glm::dot(RenderedSphere.RayOriginToSphereCenter, RenderedSphere.RayOriginToSphereCenter) - t0 * t0;
		if (d2 > RenderedSphere.RadiusSquared)
		{
			// The ray misses the sphere
			return false;
		}

		float t1 = glm::sqrt(RenderedSphere.RadiusSquared - d2);
		IntersectionDistance = t0 > t1 + Epsilon ? t0 - t1 : t0 + t1;
		return IntersectionDistance > Epsilon;
	}

	bool TraceRay(
		HScene& Scene,
		const glm::vec3& RayOrigin,
		const glm::vec3& RayDirection,
		glm::vec3& HitPosition,
		glm::vec3& HitNormal,
		glm::vec3& Albedo)
	{
		float FarClip = 1000.0f;
		float THit = FarClip;

		entt::entity HitSphereEntity = entt::null;

		for (HRenderedSphere& RenderedSphere : Scene.RenderedSpheres)
		{
			float SphereHit = 0.0f;
			if (RaySphereIntersect(RenderedSphere, RayDirection, SphereHit))
			{
				if (SphereHit < THit)
				{
					THit = SphereHit;
					HitSphereEntity = RenderedSphere.SphereEntity;
				}
			}
		}

		if (HitSphereEntity != entt::null)
		{
			HMaterial& Material = Scene.Get<HMaterial>(HitSphereEntity);
			HSphere& Sphere = Scene.Get<HSphere>(HitSphereEntity);
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(HitSphereEntity);
			HitPosition = RayOrigin + RayDirection * THit;
			HitNormal = (HitPosition - WorldTransform.Translation) / Sphere.Radius;
			Albedo = Material.Albedo;
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
			if (Distance <= 0.0f)
			{
				continue;
			}
			float LightDot = glm::max(glm::dot(Displacement / Distance, WorldNormal), 0.0f);
			float DistanceFromSurface = Distance - PointLight.Radius + 1.0f;
			if (DistanceFromSurface < 1.0f)
			{
				Color += PointLight.Color * LightDot;
			}
			else
			{
				Color += PointLight.Color * LightDot / (DistanceFromSurface * DistanceFromSurface);
			}
		}

		return Color;
	}

} // namespace

void HHoney::Render(HScene& Scene, HGUIImage& Image, entt::entity CameraEntity)
{
	const HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
	const HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

	const uint64_t Size = Image.Width * Image.Height;
	static std::vector<glm::vec3> RayDirections;
	if (RayDirections.size() != Image.Width * Image.Height)
	{
		RayDirections.clear();
		const float OneOverWidth = 1.0f / float(Image.Width);
		const float OneOverHeight = 1.0f / float(Image.Height);
		const float AspectRatio = float(Image.Height) / float(Image.Width);
		for (uint32_t Y = 0; Y < Image.Height; Y++)
		{
			for (uint32_t X = 0; X < Image.Width; X++)
			{
				RayDirections.emplace_back(glm::normalize(
					glm::vec3{ (float(X) + 0.5f - float(Image.Width) * 0.5f) * OneOverWidth,
							   (float(Y) + 0.5f - float(Image.Height) * 0.5f) * OneOverHeight * AspectRatio,
							   Camera.FocalLength }));
			}
		}
	}
	std::thread::hardware_concurrency();

	glm::vec3 RayOrigin = CameraTransform.Translation;
	for (uint32_t Pixel = 0; Pixel < Size; Pixel++)
	{
		glm::vec3 IntersectionPoint = {};
		glm::vec3 IntersectionNormal = {};
		glm::vec3 IntersectionAlbedo = {};
		if (TraceRay(
				Scene,
				RayOrigin,
				RayDirections[Pixel],
				IntersectionPoint,
				IntersectionNormal,
				IntersectionAlbedo))
		{
			Image.Pixels[Pixel] = ImGui::ColorConvertFloat4ToU32(
				glm::vec4(Lighting(Scene, IntersectionPoint, IntersectionNormal) * IntersectionAlbedo, 1.0f));
		}
		else
		{
			Image.Pixels[Pixel] = 0xFFF5B05C;
		}
	}
}

void HHoney::DrawRender(HGUIWindow& GUIWindow, HScene& Scene, entt::entity CameraEntity, HRenderWindow& RenderWindow)
{
	std::chrono::time_point Start = std::chrono::high_resolution_clock::now();

	ImVec2 Resolution = ImGui::GetContentRegionAvail();
	if (!HImGui::CreateOrUpdateImage(
			GUIWindow,
			RenderWindow.ImageIndex,
			static_cast<uint64_t>(Resolution.x),
			static_cast<uint64_t>(Resolution.y)))
	{
		return;
	}

	HGUIImage& Image = GUIWindow.Images[RenderWindow.ImageIndex];
	HHoney::Render(Scene, Image, CameraEntity);

	if (!HImGui::UploadImage(GUIWindow, RenderWindow.ImageIndex))
	{
		return;
	}
	std::chrono::time_point End = std::chrono::high_resolution_clock::now();

	ImVec2 ScreenPosition = ImGui::GetCursorScreenPos();

	HImGui::DrawImage(GUIWindow, RenderWindow.ImageIndex);

	ImGui::GetWindowDrawList()->AddText(
		ScreenPosition,
		0xFFFFFFFF,
		std::format(
			"{}x{}  {:0.2f}ms {:0.2f}FPS",
			Image.Width,
			Image.Height,
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(End - Start).count(),
			ImGui::GetIO().Framerate)
			.c_str());
}