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
	ImGui::SliderDouble4("Rotation", &RelativeTransform.Rotation.x, -1.0, 1.0, "%.3f", ImGuiSliderFlags_NoInput);
}

void HHoney::DetailsPanel(HTexture& Texture)
{
	if (ImGui::Button("Save"))
	{
		FILE* File = nullptr;

		std::filesystem::path Path = std::filesystem::path("./Textures") / Texture.Path;

		std::filesystem::create_directories(Path.parent_path());

		if (fopen_s(&File, Path.string().c_str(), "wb") == 0)
		{
			const uint64_t Size = Texture.Resolution.x * Texture.Resolution.y * 4;
			fwrite(Texture.Data.data(), Size, 1, File);
			fclose(File);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load"))
	{
		FILE* File = nullptr;

		std::filesystem::path Path = std::filesystem::path("./Textures") / Texture.Path;
		if (fopen_s(&File, Path.string().c_str(), "rb") == 0)
		{
			const uint64_t Size = Texture.Resolution.x * Texture.Resolution.y * 4;
			fread(Texture.Data.data(), Size, 1, File);
			fclose(File);
			Texture.Version++;
		}
	}

	std::string Path = Texture.Path.string();
	char PathBuffer[MAX_PATH] = {};
	strcpy_s(PathBuffer, Path.c_str());

	if (ImGui::InputText("Path", PathBuffer, MAX_PATH))
	{
		Texture.Path = PathBuffer;
	}

	ImGui::Text("Width %d", Texture.Resolution.x);
	ImGui::Text("Height %d", Texture.Resolution.y);

	static float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ImGui::ColorPicker4("Color", Color);

	static float Scale = 8.0f;
	Scale += ImGui::GetIO().MouseWheel;

	glm::vec2 CursorPos = ImGui::GetCursorScreenPos();

	ImGui::Image(
		reinterpret_cast<ImTextureID>(Texture.Descriptor.GPUDescriptorHandle.ptr),
		ImVec2{ static_cast<float>(Texture.Resolution.x) * Scale, static_cast<float>(Texture.Resolution.y) * Scale },
		{ 0, 0 },
		{ 1, 1 },
		ImVec4{ 1, 1, 1, 1 });

	glm::vec2 MousePosInImage = glm::vec2(ImGui::GetMousePos()) - CursorPos;
	glm::uvec2 PixelInImage = glm::floor(MousePosInImage / Scale);

	if (PixelInImage.x >= 0u && PixelInImage.y >= 0u && PixelInImage.x < Texture.Resolution.x
		&& PixelInImage.y < Texture.Resolution.y)
	{
		uint64_t Pixel = PixelInImage.y * Texture.Resolution.x + PixelInImage.x;

		if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			Texture.Data.data()[Pixel * 4 + 0] = glm::clamp(Color[0] * 255.0f, 0.0f, 255.0f);
			Texture.Data.data()[Pixel * 4 + 1] = glm::clamp(Color[1] * 255.0f, 0.0f, 255.0f);
			Texture.Data.data()[Pixel * 4 + 2] = glm::clamp(Color[2] * 255.0f, 0.0f, 255.0f);
			Texture.Data.data()[Pixel * 4 + 3] = glm::clamp(Color[3] * 255.0f, 0.0f, 255.0f);
			Texture.Version++;
		}
		else if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Right])
		{
			Color[0] = static_cast<float>(Texture.Data.data()[Pixel * 4 + 0]) / 255.0f;
			Color[1] = static_cast<float>(Texture.Data.data()[Pixel * 4 + 1]) / 255.0f;
			Color[2] = static_cast<float>(Texture.Data.data()[Pixel * 4 + 2]) / 255.0f;
			Color[3] = static_cast<float>(Texture.Data.data()[Pixel * 4 + 3]) / 255.0f;
		}
	}

	ImGui::SetTooltip(std::format(
						  "MousePos = [{}, {}] -> [{}, {}]",
						  MousePosInImage.x,
						  MousePosInImage.y,
						  PixelInImage.x,
						  PixelInImage.y)
						  .c_str());
}