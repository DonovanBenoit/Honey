cmake_minimum_required(VERSION 3.13)
project(Hive)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_internal.h
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_win32.cpp
	imgui/backends/imgui_impl_dx12.cpp)
	
add_library(ImGui ${SOURCE_FILES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Honey)

target_include_directories(ImGui PUBLIC imgui)
target_include_directories(ImGui PUBLIC imgui/backends)

# What to do about other platforms?
include_directories(ImGui "C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")
target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/d3d12.lib")
target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxgi.lib")
target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxguid.lib")
target_link_libraries(ImGui PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/propsys.lib")