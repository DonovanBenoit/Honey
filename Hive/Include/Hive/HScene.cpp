#include "HScene.h"

#include <HImGui.h>
#include <format>
#include <glm/gtc/random.hpp>
#include <glm/gtx/color_space.hpp>
#include <imgui.h>

entt::entity HScene::CreateCamera()
{
	entt::entity CameraEntity = Registry.create();
	HCamera& Camera = Registry.emplace<HCamera>(CameraEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(CameraEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(CameraEntity);
	return CameraEntity;
}

entt::entity HScene::CreateSphere()
{
	entt::entity SphereEntity = Registry.create();
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
	entt::entity SDFEntity = Registry.create();
	HSDF& SDF = Registry.emplace<HSDF>(SDFEntity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(SDFEntity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(SDFEntity);
	HMaterial& Material = Registry.emplace<HMaterial>(SDFEntity);
	RenderedMaterials.emplace_back();
	return SDFEntity;
}

entt::entity HScene::CreatePointLight()
{
	entt::entity Entity = Registry.create();
	HPointLight& PointLight = Registry.emplace<HPointLight>(Entity);
	HWorldTransform& WorldTransform = Registry.emplace<HWorldTransform>(Entity);
	HRelativeTransform& RelativeTransform = Registry.emplace<HRelativeTransform>(Entity);
	return Entity;
}

entt::entity HScene::CreateTexture(const glm::uvec2& Resolution)
{
	entt::entity Entity = Registry.create();
	HTexture& Texture = Registry.emplace<HTexture>(Entity);
	Texture.Resolution = Resolution;
	Texture.Data.resize(Resolution.x * Resolution.y * 4);
	return Entity;
}

void HHoney::UpdateScene(HScene& Scene, entt::entity CameraEntity)
{
	auto TransformView = Scene.Registry.view<HRelativeTransform, HWorldTransform>();
	for (entt::entity TransformEntity : TransformView)
	{
		HRelativeTransform& RelativeTransform = Scene.Get<HRelativeTransform>(TransformEntity);
		HWorldTransform& WorldTransform = Scene.Get<HWorldTransform>(TransformEntity);

		WorldTransform.Translation = RelativeTransform.Translation;
		WorldTransform.Rotation = RelativeTransform.Rotation;
	}

	const HWorldTransform& CameraTransform = Scene.Get<HWorldTransform>(CameraEntity);

	uint64_t MaterialIndex = 0;
	uint64_t SphereIndex = 0;
	Scene.Registry.view<HSphere, HMaterial, HWorldTransform>().each(
		[&](entt::entity Entity, HSphere& Sphere, HMaterial& Material, HWorldTransform& WorldTransform) {
			HRenderedSphere& RenderedSphere = Scene.RenderedSpheres[SphereIndex++];
			RenderedSphere.RadiusSquared = Sphere.Radius * Sphere.Radius;
			RenderedSphere.SphereCenter = glm::vec4(WorldTransform.Translation, 0.0f);
			RenderedSphere.RayOriginToSphereCenter =
				glm::vec4(WorldTransform.Translation - CameraTransform.Translation, 0.0f);
			RenderedSphere.MaterialIndex = MaterialIndex;
			Scene.RenderedMaterials[MaterialIndex++] = Material;
		});

	Scene.RenderedSDFs.clear();

	Scene.Registry.view<HSDF, HMaterial, HWorldTransform>().each(
		[&](entt::entity Entity, HSDF& SDF, HMaterial& Material, HWorldTransform& WorldTransform) {
			SDF.PositionRadius = glm::vec4(WorldTransform.Translation, SDF.PositionRadius.a);
			Scene.RenderedSDFs.push_back(SDF);
		});
}

void HHoney::SceneEditor(HScene& Scene)
{
	ImGui::Text("Cameras");

	static entt::entity SelectedEntity = entt::null;

	Scene.Registry.view<HCamera>().each([&](entt::entity Entity, HCamera& Camera) {
		if (ImGui::TreeNodeEx(std::format("Camera_{}", (uint32_t)Entity).c_str(), ImGuiTreeNodeFlags_Leaf))
		{
			if (ImGui::IsItemClicked())
			{
				SelectedEntity = Entity;
			}
			ImGui::TreePop();
		}
	});

	ImGui::Text("Materials");

	ImGui::Text("Textures");
	Scene.Registry.view<HTexture>().each([&](entt::entity Entity, HTexture& Texture) {
		if (ImGui::TreeNodeEx(std::format("Texture_{}", (uint32_t)Entity).c_str(), ImGuiTreeNodeFlags_Leaf))
		{
			if (ImGui::IsItemClicked())
			{
				SelectedEntity = Entity;
			}
			ImGui::TreePop();
		}
	});

	for (HTreeView& TreeView : Scene.TreeViews)
	{
		TreeView(Scene, SelectedEntity);
	}

	ImGui::Text("Details");

	if (SelectedEntity != entt::null)
	{
		for (HDetailsPannel& DetailsPannel : Scene.DetailsPannels)
		{
			DetailsPannel(SelectedEntity, Scene);
		}

		// Camera
		if (Scene.Has<HCamera>(SelectedEntity))
		{
			DetailsPanel(Scene.Get<HCamera>(SelectedEntity));
		}

		// Transform
		if (Scene.Has<HRelativeTransform>(SelectedEntity))
		{
			DetailsPanel(Scene.Get<HRelativeTransform>(SelectedEntity));
		}

		// Texture
		if (Scene.Has<HTexture>(SelectedEntity))
		{
			DetailsPanel(Scene.Get<HTexture>(SelectedEntity));
		}
	}
}

void HHoney::DetailsPanel(HCamera& Camera)
{
	ImGui::Text("Camera");
	ImGui::SliderFloat("Focal Length", &Camera.FocalLength, 0.01f, 4.0f);
}

void HHoney::DetailsPanel(HRelativeTransform& RelativeTransform)
{
	ImGui::Text("RelativeTransform");
	ImGui::SliderDouble3("Translation", &RelativeTransform.Translation.x, -10.0, 10.0);
}

void HHoney::DetailsPanel(HTexture& Texture)
{
	ImGui::Text("Width %d", Texture.Resolution.x);
	ImGui::Text("Height %d", Texture.Resolution.y);

	ImGui::Image(
		reinterpret_cast<ImTextureID>(Texture.Descriptor.GPUDescriptorHandle.ptr),
		ImVec2{ static_cast<float>(Texture.Resolution.x), static_cast<float>(Texture.Resolution.y) },
		{ 0, 0 },
		{ 1, 1 },
		ImVec4{ 1, 1, 1, 1 });
}