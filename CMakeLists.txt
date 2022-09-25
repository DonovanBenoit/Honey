cmake_minimum_required(VERSION 3.13)
project(HoneyBuns)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES Sources/main.cpp)

add_executable(Honey ${SOURCE_FILES})

add_subdirectory(Libraries/glm)

target_link_libraries(Honey PRIVATE glm)