#pragma once

#include <array>

#include <SFML/Graphics.hpp>
#include <imgui.h>

#include "math/Types.hpp"

struct UiState {
    float ws{};
    float yaw{};
    float pitch{};
    float fovDeg{};
    float f{};
    float sceneScale{};
    float aspect{};
    unsigned int windowW{};
    unsigned int windowH{};

    Vec3 v1{};
    Vec3 v2{};
    Vec3 v3{};
    Vec3 u1{};
    Vec3 u2{};
    Vec3 u3{};
    Vec3 a{};
    Vec3 b{};
    Vec3 w{};

    Mat4 MV_base{};
    Mat4 MV_vectors{};
    Mat4 P{};

    std::array<Vec3, 7> tipVecs{};
};

void ShowMatrixLab(const UiState& state);
