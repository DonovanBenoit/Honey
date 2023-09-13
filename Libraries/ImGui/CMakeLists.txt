cmake_minimum_required(VERSION 3.13)
project(Hive)
set(CMAKE_CXX_STANDARD 20)

add_library(ImGui
 Source/HImGui.cpp
 Source/HImGui.h
 Source/HWindow.cpp
 Source/HWindow.h)

target_link_libraries(ImGui PRIVATE glm)

target_include_directories(ImGui PUBLIC Source)
target_include_directories(ImGui PUBLIC imgui)
target_include_directories(ImGui PUBLIC imgui/backends)
target_include_directories(ImGui PUBLIC ThirdParty/Windows)

target_link_libraries(ImGui PRIVATE ${GLFW_LIBRARIES})
target_include_directories(ImGui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include/")

if (MSVC)
	include_directories(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")
	target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/d3d12.lib")
	target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxgi.lib")
	target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxguid.lib")
	target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/propsys.lib")
	target_link_libraries(ImGui PRIVATE DirectX)
endif()