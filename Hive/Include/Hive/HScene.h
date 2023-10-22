#pragma once

#include <HDirectX.h>

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

struct HMesh
{
	std::vector<glm::vec3> Positions;

	// 3 * TriangleCount in size
	std::vector<uint32_t> Indicies;
};

struct HTexture
{
	uint64_t Version = 0;
	std::vector<uint8_t> Data{};
	glm::uvec2 Resolution{};

	HResource Resource{};
	HDescriptor Descriptor{};
};

struct HRenderedSphere
{
	glm::vec4 SphereCenter = {};
	glm::vec4 RayOriginToSphereCenter = {};
	float RadiusSquared = 0.25f;
	uint32_t MaterialIndex = 0;
};
static_assert(sizeof(HRenderedSphere) % 4 == 0);

enum class HSDFShape
{
	Sphere
};

struct HSDF
{
	glm::vec4 PositionRadius = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct HRenderedScene
{
	glm::vec3 RayOrigin = { 0.0f, 0.0f, -1.0f };
	uint32_t SphereCount = 0;
	uint32_t SDFCount = 0;
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
	glm::dquat Rotation = glm::angleAxis(0.0, glm::dvec3{ 0.0, 1.0, 0.0 });
	glm::dvec3 Scale = { 1.0, 1.0, 1.0 };
};

struct HWorldTransform
{
	glm::dvec3 Translation = {};
	glm::dquat Rotation = glm::angleAxis(0.0, glm::dvec3{ 0.0, 1.0, 0.0 });
	glm::dvec3 Scale = { 1.0, 1.0, 1.0 };
};

struct HNode
{
	entt::entity Parent = entt::null;
	entt::entity FirstChild = entt::null;
	entt::entity LeftSibbling = entt::null;
	entt::entity RightSibbling = entt::null;
};

struct HScene;

using HDetailsPannel = void (*)(entt::entity Entity, HScene& Scene);
using HTreeView = void (*)(HScene& Scene, entt::entity& SelectedEntity);

struct HScene
{
	entt::registry Registry{};

	entt::entity CreateCamera();
	entt::entity CreateSphere();
	entt::entity CreateSDF();
	entt::entity CreatePointLight();
	entt::entity CreateTexture(const glm::uvec2& Resolution);

	template<typename T>
	T& Get(entt::entity Entity)
	{
		return Registry.get<T>(Entity);
	}

	template<typename T>
	bool Has(entt::entity Entity)
	{
		return Registry.any_of<T>(Entity);
	}

	std::vector<HSDF> RenderedSDFs{};
	std::vector<HRenderedSphere> RenderedSpheres{};
	std::vector<HMaterial> RenderedMaterials{};
	std::vector<HTexture> Textures;

	std::vector<HTreeView> TreeViews{};
	std::vector<HDetailsPannel> DetailsPannels{};
};

namespace HHoney
{
	void UpdateScene(HScene& Scene, entt::entity CameraEntity);

	void SceneEditor(HScene& Scene);

	void DetailsPanel(HCamera& Camera);

	void DetailsPanel(HRelativeTransform& RelativeTransform);

	void DetailsPanel(HTexture& Texture);
} // namespace HHoney