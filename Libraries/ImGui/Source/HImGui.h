#pragma once

// #define ImTextureID ImU64

#include <glm/glm.hpp>

#define IM_VEC2_CLASS_EXTRA \
	constexpr ImVec2(const glm::vec2& f) \
		: x(f.x) \
		, y(f.y) \
	{ \
	} \
	operator glm::vec2() const \
	{ \
		return glm::vec2(x, y); \
	}

#define IM_VEC4_CLASS_EXTRA \
	constexpr ImVec4(const glm::vec4& f) \
		: x(f.x) \
		, y(f.y) \
		, z(f.z) \
		, w(f.w) \
	{ \
	} \
	operator glm::vec4() const \
	{ \
		return glm::vec4(x, y, z, w); \
	}

#include "imgui.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"