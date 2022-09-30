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

namespace HTracer
{
	void DrawTracer(const glm::vec2& Resolution);
}