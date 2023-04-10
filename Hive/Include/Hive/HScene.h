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

template<uint64_t Dimensions = 3>
struct HMesh
{
	std::vector<glm::vec<Dimensions, float>> Positions;
	std::vector<glm::vec<Dimensions, float>> Normals;
};

struct HRenderedSphere
{
	float RadiusSquared = 0.25f;
	glm::vec3 RayOriginToSphereCenter = {};
	entt::entity SphereEntity = entt::null;
};

struct HMaterial
{
	glm::vec3 Albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	float Roughness = 0.0f;
	float Metallic = 0.0f;
};

struct HPointLight
{
	glm::vec3 Color = glm::vec3(10.0f, 10.0f, 10.0f);
	float Radius = 0.01f;
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

	std::vector<HRenderedSphere> RenderedSpheres{};
};

namespace HHoney
{
	void DefaultScene(HScene& Scene);

	void UpdateScene(HScene& Scene, entt::entity CameraEntity);
}