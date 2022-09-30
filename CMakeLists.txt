cmake_minimum_required(VERSION 3.13)
project(Hive)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
	Honey/main.cpp
	Honey/HTracer.h
	Honey/HTracer.cpp)
	
add_executable(Honey ${SOURCE_FILES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Honey)

add_subdirectory(Libraries/glm)
add_subdirectory(Libraries/ImGui)

target_link_libraries(Honey PRIVATE ImGui)
target_link_libraries(Honey PRIVATE glm)

# What to do about other platforms?
include_directories(Honey "C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/d3d12.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxgi.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxguid.lib")
target_link_libraries(Honey PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/propsys.lib")