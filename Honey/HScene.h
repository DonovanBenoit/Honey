#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

struct HCamera
{
	glm::vec3 Translation{};
	float FocalLength = 1.0f;
	int32_t Width = 16;
	int32_t Height = 16;
};

struct HSphere
{
	float Radius = 0.5f;
};

struct HMaterial
{
	glm::vec3 Albedo;
};

struct HPointLight
{
	glm::vec3 Color = glm::vec3(10.0f, 10.0f, 10.0f);
	float Radius = 0.05f;
};

struct HRelativeTransform
{
	glm::dvec3 Translation = {};
	glm::dvec3 Rotation = { 0.0f, 0.0f, 0.0 };
};

struct HWorldTransform
{
	glm::vec3 Translation = {};
	glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct HScene
{
	entt::registry Registry{};

	entt::entity RootEntity = Registry.create();

	std::vector<entt::entity> Cameras{};
	std::vector<entt::entity> Spheres{};
	std::vector<entt::entity> PointLights{};

	entt::entity CreateCamera();
	entt::entity CreateSphere();
	entt::entity CreatePointLight();

	template<typename T>
	T& Get(entt::entity Entity)
	{
		return Registry.get<T>(Entity);
	}
};

namespace HHoney
{
	void DefaultScene(HScene& Scene);

	void UpdateScene(HScene& Scene);
}