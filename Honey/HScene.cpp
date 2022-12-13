#include "HScene.h"


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
	return SphereEntity;
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
	entt::entity CameraEntity = Scene.CreateCamera();
	HRelativeTransform& CameraTransform = Scene.Get<HRelativeTransform>(CameraEntity);
	CameraTransform.Translation = { 0.0, 0.0, -2.0 };
	entt::entity SphereEntity = Scene.CreateSphere();
	entt::entity PointLightEntity = Scene.CreatePointLight();
	HRelativeTransform& PointLightTransform = Scene.Get<HRelativeTransform>(PointLightEntity);
	PointLightTransform.Translation = { 0.0, -1.0, -4.0 };
}

void HHoney::UpdateScene(HScene& Scene)
{
	auto TransformView = Scene.Registry.view<HRelativeTransform, HWorldTransform>();
	for (entt::entity TransformEntity : TransformView)
	{
		HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(TransformEntity);
		HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(TransformEntity);

		WorldTransform.Translation = RelativeTransform.Translation;
		WorldTransform.Rotation = glm::quat(RelativeTransform.Rotation);
	}
}