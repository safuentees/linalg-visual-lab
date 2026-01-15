cmake_minimum_required(VERSION 4.1)
project(projection_3d_2d)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(glm CONFIG REQUIRED)

add_executable(projection_3d_2d main.cpp)

target_link_libraries(projection_3d_2d
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        glm::glm
)