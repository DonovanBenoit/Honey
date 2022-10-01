cmake_minimum_required(VERSION 3.13)
project(DirectX)
set(CMAKE_CXX_STANDARD 20)

add_library(DirectX
 Source/HDirectX.cpp
 Source/HDirectX.h)

target_link_libraries(DirectX PRIVATE glm)

target_include_directories(DirectX PUBLIC Source)

include_directories(DirectX PUBLIC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um")
target_link_libraries(DirectX PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/d3d12.lib")
target_link_libraries(DirectX PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxgi.lib")
target_link_libraries(DirectX PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/dxguid.lib")
target_link_libraries(DirectX PUBLIC "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64/propsys.lib")
