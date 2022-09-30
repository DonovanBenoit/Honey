#pragma once

#include <glm/glm.hpp>

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

struct HDirectionalLight
{
	glm::vec3 Intensity = glm::vec3(10.0f, 10.0f, 10.0f);
	glm::vec3 Direction = glm::vec3(0.0f, 1.0f, 0.0f);
};