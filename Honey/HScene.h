#pragma once

#include <glm/glm.hpp>

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
	glm::vec3 Translation;
	float Radius;
};

struct HPointLight
{
	glm::vec3 SurfaceIntensity = glm::vec3(10.0f, 10.0f, 10.0f);
	glm::vec3 Position = glm::vec3(0.0f, 1.0f, 0.0f);
	float Radius = 0.1f;
};

struct HScene
{
	HCamera Camera{};
	std::vector<HSphere> Spheres{};
	std::vector<HPointLight> PointLights{};
};