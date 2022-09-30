cmake_minimum_required(VERSION 3.13)
project(Hive)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
	Honey/main.cpp
	Honey/HTracer.h
	Honey/HTracer.cpp
	Libraries/imgui/imgui.cpp
	Libraries/imgui/imgui_demo.cpp
	Libraries/imgui/imgui_draw.cpp
	Libraries/imgui/imgui_internal.h
	Libraries/imgui/imgui_tables.cpp
	Libraries/imgui/imgui_widgets.cpp
	Libraries/imgui/backends/imgui_impl_win32.cpp
	Libraries/imgui/backends/imgui_impl_dx12.cpp)
	
add_executable(Honey ${SOURCE_FILES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Honey)

add_subdirectory(Libraries/glm)

target_link_libraries(Honey PRIVATE glm)
target_include_directories(Honey PRIVATE Libraries/imgui)
target_include_directories(Honey PRIVATE Libraries/imgui/backends)

# What to do about other platforms?
include_directories(Honey "C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/d3d12.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxgi.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxguid.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/propsys.lib")