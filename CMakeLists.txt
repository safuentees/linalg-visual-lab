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

# Google Test for unit testing
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(projection_3d_2d
        src/main.cpp
        src/ui/MatrixLabUI.cpp
        src/ui/MatrixLabUI.hpp
        src/app/App.cpp
        src/app/App.hpp
        src/render/Projection.cpp
        src/render/Projection.hpp
        src/render/Mesh.cpp
        src/render/Mesh.hpp
        src/math/Camera.cpp
        src/math/Camera.hpp
        src/math/Basis.cpp
        src/math/Basis.hpp
        src/math/Types.hpp
        src/math/Quaternion.h
        src/math/Quaternion.cpp
        src/math/Shadow.cpp
        src/math/Shadow.h
        src/math/Lighting.cpp
        src/math/Lighting.h)

target_include_directories(projection_3d_2d
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

# Test executable
enable_testing()

add_executable(quaternion_tests
        tests/QuaternionTest.cpp
        src/math/Quaternion.cpp
)

target_include_directories(quaternion_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(quaternion_tests
        PRIVATE
        GTest::gtest_main
        glm::glm
)

include(GoogleTest)
gtest_discover_tests(quaternion_tests)
