#include "HScene.h"

#include <glm/gtc/random.hpp>
#include <glm/gtx/color_space.hpp>

entt::entity HScene::CreateCamera()
{
	entt::entity CameraEntity = Cameras.emplace_back(Registry.create());
	HCamera& Camera = Registry.emplace<HCamera>(CameraEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(CameraEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(CameraEntity);
	return CameraEntity;
}

entt::entity HScene::CreateSphere()
{
	entt::entity SphereEntity = Spheres.emplace_back(Registry.create());
	HSphere& Sphere = Registry.emplace<HSphere>(SphereEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(SphereEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(SphereEntity);
	HMaterial& Material = Registry.emplace<HMaterial>(SphereEntity);
	RenderedSpheres.emplace_back();
	RenderedMaterials.emplace_back();
	return SphereEntity;
}

entt::entity HScene::CreateSDF()
{
	entt::entity SDFEntity = Spheres.emplace_back(Registry.create());
	HSDF& SDF = Registry.emplace<HSDF>(SDFEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(SDFEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(SDFEntity);
	HMaterial& Material = Registry.emplace<HMaterial>(SDFEntity);
	SDFs.emplace_back();
	RenderedMaterials.emplace_back();
	return SDFEntity;
}

entt::entity HScene::CreatePointLight()
{
	entt::entity PointLightEntity = PointLights.emplace_back(Registry.create());
	HPointLight& PointLight = Registry.emplace<HPointLight>(PointLightEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(PointLightEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(PointLightEntity);
	return PointLightEntity;
}

void HHoney::DefaultScene(HScene& Scene)
{
	srand(1925);

	entt::entity CameraEntity = Scene.CreateCamera();
	HRelativeTransform& CameraTransform = Scene.Get<HRelativeTransform>(CameraEntity);
	CameraTransform.Translation = { 0.0, 0.0, -10.0 };
	HCamera& Camera = Scene.Get<HCamera>(CameraEntity);
	Camera.FocalLength = 0.5f;

	entt::entity PointLightEntity = Scene.CreatePointLight();
	HRelativeTransform& PointLightTransform = Scene.Get<HRelativeTransform>(PointLightEntity);
	PointLightTransform.Translation = { 0.0, -1.0, -4.0 };

	int32_t SphereCount = 128;
	for (int32_t SphereIndex = 0; SphereIndex < SphereCount; SphereIndex++)
	{
		entt::entity SphereEntity = Scene.CreateSphere();
		HMaterial& Material = Scene.Get<HMaterial>(SphereEntity);
		HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(SphereEntity);
		HSphere& Sphere = Scene.Get<HSphere>(SphereEntity);

		Sphere.Radius = glm::linearRand(0.2f, 2.0f);
		RelativeTransform.Translation = glm::sphericalRand(1.0f) * glm::linearRand(0.1f, 10.0f);

		Material.Albedo = glm::abs(glm::sphericalRand(1.0f));
	}

	int32_t SDFCount = 128;
	for (int32_t SDFIndex = 0; SDFIndex < SDFCount; SDFIndex++)
	{
		entt::entity SDFEntity = Scene.CreateSDF();
		HMaterial& Material = Scene.Get<HMaterial>(SDFEntity);
		HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(SDFEntity);
		HSDF& SDF = Scene.Get<HSDF>(SDFEntity);

		SDF.Radius = glm::linearRand(0.2f, 2.0f);
		RelativeTransform.Translation = glm::sphericalRand(1.0f) * glm::linearRand(0.1f, 10.0f);

		Material.Albedo = glm::abs(glm::sphericalRand(1.0f));
	}
}

void HHoney::UpdateScene(HScene& Scene, entt::entity CameraEntity)
{
	auto TransformView = Scene.Registry.view<HRelativeTransform, HWorldTransform>();
	for (entt::entity TransformEntity : TransformView)
	{
		HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(TransformEntity);
		HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(TransformEntity);

		WorldTransform.Translation = RelativeTransform.Translation;
		WorldTransform.Rotation = glm::quat(RelativeTransform.Rotation);
	}

	const HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

	uint64_t MaterialIndex = 0;
	uint64_t SphereIndex = 0;
	Scene.Registry.view<HSphere, HMaterial, HWorldTransform>().each(
		[&](entt::entity Entity, HSphere& Sphere, HMaterial& Material, HWorldTransform& WorldTransform) {
			HRenderedSphere& RenderedSphere = Scene.RenderedSpheres[SphereIndex++];
			RenderedSphere.RadiusSquared = Sphere.Radius * Sphere.Radius;
			RenderedSphere.SphereCenter = glm::vec4(WorldTransform.Translation, 0.0f);
			RenderedSphere.RayOriginToSphereCenter = glm::vec4(WorldTransform.Translation - CameraTransform.Translation, 0.0f);
			RenderedSphere.MaterialIndex = MaterialIndex;
			Scene.RenderedMaterials[MaterialIndex++] = Material;
		});

	Scene.Registry.view<HSDF, HMaterial, HWorldTransform>().each(
		[&](entt::entity Entity, HSDF& SDF, HMaterial& Material, HWorldTransform& WorldTransform) {
			SDF.Position = WorldTransform.Translation;
		});
}