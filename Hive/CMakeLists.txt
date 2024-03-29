cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
	Include/Hive/HRender.h
	Include/Hive/HRender.cpp
	Include/Hive/HScene.h
	Include/Hive/HScene.cpp
	)

# Shaders
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Hive/*.hlsl")
foreach(SHADER_FILE ${SHADER_FILES})
	set_source_files_properties(${SHADER_FILE} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach(SHADER_FILE)

add_library(Hive ${SOURCE_FILES} ${SHADER_FILES})

target_include_directories(Hive
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
)


add_compile_definitions(NOMINMAX)

target_include_directories(Hive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/entt/src/)

target_link_libraries(Hive PRIVATE ImGui)
target_link_libraries(Hive PRIVATE glm)
if (MSVC)
	target_link_libraries(Hive PRIVATE DirectX)
endif()


target_link_libraries(Hive PRIVATE glfw)
target_include_directories(Hive PRIVATE "${GLFW_SOURCE_DIR}")