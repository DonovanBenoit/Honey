#include "HTracer.h"

#include "HScene.h"
#include "HWindow.h"

#include <HImGui.h>
#include <chrono>
#include <entt/entt.hpp>
#include <format>
#include <glm/gtx/intersect.hpp>
#include <imgui.h>

namespace ChatGPT
{
	using namespace glm;

	// This function should calculate the signed distance from the given position to the nearest surface in the scene
	float signed_distance(const vec3& position)
	{
		// Set the center of the sphere to (0, 0, 0) and the radius to 1
		vec3 center = vec3(0, 0, 0);
		float radius = 1.0f;

		// Calculate the distance from the position to the center of the sphere
		float distance = length(position - center);

		// Return the signed distance (distance minus the radius of the sphere)
		return distance - radius;
	}

	vec3 gradient(const vec3& position)
	{
		// Set the finite difference step size
		float h = 0.001f;

		// Calculate the partial derivatives of the signed distance function
		float dx = signed_distance(vec3(position.x + h, position.y, position.z))
				   - signed_distance(vec3(position.x - h, position.y, position.z));
		float dy = signed_distance(vec3(position.x, position.y + h, position.z))
				   - signed_distance(vec3(position.x, position.y - h, position.z));
		float dz = signed_distance(vec3(position.x, position.y, position.z + h))
				   - signed_distance(vec3(position.x, position.y, position.z - h));

		// Return the gradient vector
		return vec3(dx, dy, dz);
	}

	void ray_march(
		const vec3& start,
		const vec3& direction,
		float max_distance,
		float min_distance,
		int max_iterations,
		vec3& position,
		float& distance,
		vec3& normal)
	{
		// Set the current position to the start position
		position = start;
		// Set the current distance to the maximum distance
		distance = max_distance;

		// Set the iteration counter to 0
		int iterations = 0;

		// While the current distance is greater than the minimum distance and
		// the iteration counter is less than the maximum number of iterations:
		while (distance > min_distance && iterations < max_iterations)
		{
			// Calculate the signed distance from the current position to the nearest surface
			distance = signed_distance(position);

			// Move the current position along the direction vector by the current distance
			position = position + distance * direction;

			// Increment the iteration counter
			iterations++;
		}
		// Calculate the surface normal at the final position
		normal = normalize(gradient(position));
	}

	vec3 trace(const vec3& origin, const vec3& direction, vec3& normal)
	{
		// Set the maximum distance to march to 100
		float max_distance = 100.0f;
		// Set the minimum distance to stop at 0.01
		float min_distance = 0.01f;
		// Set the maximum number of iterations to 1000
		int max_iterations = 1000;

		// Initialize the position and distance variables
		vec3 position;
		float distance;

		// March the ray and get the final position and distance
		ray_march(origin, direction, max_distance, min_distance, max_iterations, position, distance, normal);

		// Return the final position
		return position;
	}
} // namespace ChatGPT

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

		auto Spheres = Scene.Registry.view<HSphere>();
		for (entt::entity SphereEntity : Spheres)
		{
			HSphere& Sphere = Scene.Get<HSphere>(SphereEntity);
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(SphereEntity);

			float SphereHit = FarClip;

			// ChatGPT::ray_march(RayOrigin, RayDirection, FarClip, 0.1f, 32, HitPosition, THit, HitNormal);

			if (glm::intersectRaySphere(RayOrigin, RayDirection, WorldTransform.Translation, Sphere.Radius, SphereHit))
			{
				if (SphereHit < THit)
				{
					THit = SphereHit;

					HMaterial& Material = Scene.Get<HMaterial>(SphereEntity);

					HitPosition = RayOrigin + RayDirection * THit;
					HitNormal = (HitPosition - WorldTransform.Translation) / Sphere.Radius;
					float Length = glm::length(HitNormal);
					HitNormal /= Length;
					Albedo = Material.Albedo;
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

	void Update(HScene& Scene)
	{
		auto Entities = Scene.Registry.view<HWorldTransform, HRelativeTransform>();
		for (entt::entity Entity : Entities)
		{
			HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(Entity);
			HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(Entity);
			WorldTransform.Translation = RelativeTransform.Translation;
			WorldTransform.Rotation = glm::quat(RelativeTransform.Rotation);
		}
	}

	void DrawTracer(HGUIWindow& GUIWindow, HScene& Scene, HTracer& Tracer)
	{
		std::chrono::time_point Start = std::chrono::high_resolution_clock::now();

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

		const float AspectRatio = float(Image.Height) / float(Image.Width);
		entt::entity CameraEntity = Scene.Cameras[0];
		HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
		HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

		glm::vec3 IntersectionPoint = {};
		glm::vec3 IntersectionNormal = {};
		glm::vec3 IntersectionAlbedo = {};

		glm::vec3 RayOrigin = CameraTransform.Translation;
		for (uint32_t Y = 0; Y < Image.Height; Y++)
		{
			for (uint32_t X = 0; X < Image.Width; X++)
			{
				glm::vec3 RayDirection = glm::normalize(
					glm::vec3{ (float(X) + 0.5f - float(Image.Width) * 0.5f) / float(Image.Width),
							   (float(Y) + 0.5f - float(Image.Height) * 0.5f) / float(Image.Height) * AspectRatio,
							   Camera.FocalLength });

				glm::vec3 Color;
				if (TraceRay(Scene, RayOrigin, RayDirection, IntersectionPoint, IntersectionNormal, IntersectionAlbedo))
				{
					Color = Lighting(Scene, IntersectionPoint, IntersectionNormal) * IntersectionAlbedo;
				}
				else
				{
					Color = { 0.36f, 0.69f, 0.96f };
				}

				Image.Pixels[Y * Image.Width + X] = ImGui::ColorConvertFloat4ToU32(glm::vec4(Color, 1.0f));
			}
		}

		if (!HImGui::UploadImage(GUIWindow, Tracer.ImageIndex))
		{
			return;
		}
		std::chrono::time_point End = std::chrono::high_resolution_clock::now();

		ImVec2 ScreenPosition = ImGui::GetCursorScreenPos();

		HImGui::DrawImage(GUIWindow, Tracer.ImageIndex);

		ImGui::GetWindowDrawList()->AddText(
			ScreenPosition,
			0xFFFFFFFF,
			std::format(
				"{}x{}  {:0.2f}ms",
				Image.Width,
				Image.Height,
				std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(End - Start).count())
				.c_str());
	}
} // namespace

void HHoney::DrawHoney(const glm::vec2& Resolution, HGUIWindow& GUIWindow, HScene& Scene, HTracer& Tracer)
{
	ImGui::SetNextWindowPos({});
	ImGui::SetNextWindowSize(Resolution);

	Update(Scene);

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
