cmake_minimum_required(VERSION 3.13)
project(Apiary)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
	Honey/main.cpp
	Honey/HTracer.h
	Honey/HTracer.cpp
	)
	
add_executable(Honey ${SOURCE_FILES})

add_subdirectory(Hive)
target_link_libraries(Honey PRIVATE Hive)

add_compile_definitions(NOMINMAX)

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Honey)
endif()

target_include_directories(Honey PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/entt/src/)

add_subdirectory(Libraries/glm)
add_subdirectory(Libraries/glfw)
add_subdirectory(Libraries/ImGui)
if (MSVC)
	add_subdirectory(Libraries/DirectX)
endif()

target_link_libraries(Honey PRIVATE ImGui)
target_link_libraries(Honey PRIVATE glm)
if (MSVC)
	target_link_libraries(Honey PRIVATE DirectX)
endif()


target_link_libraries(Honey PRIVATE glfw)
