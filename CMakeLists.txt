cmake_minimum_required(VERSION 4.1)
project(projection_3d_2d)

set(CMAKE_CXX_STANDARD 20)

# Use shared SFML libs (Homebrew only has shared libs)
set(SFML_STATIC_LIBRARIES OFF)

# Provide SFML_DIR for Homebrew SFML
if(APPLE)
    set(SFML_DIR "/opt/homebrew/lib/cmake/SFML")
endif()

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(glm CONFIG REQUIRED)

include(FetchContent)

# Fetch ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.1
)

FetchContent_Declare(
        imgui_sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v3.0
)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

set(IMGUI_DIR "${imgui_SOURCE_DIR}" CACHE PATH "Path to ImGui sources" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Disable SFML find in ImGui-SFML" FORCE)

FetchContent_MakeAvailable(imgui_sfml)

add_executable(projection_3d_2d main.cpp ui.cpp)

target_link_libraries(projection_3d_2d
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        glm::glm
        ImGui-SFML::ImGui-SFML
)

# On macOS you might need to link OpenGL:
target_link_libraries(projection_3d_2d PRIVATE "-framework OpenGL")
