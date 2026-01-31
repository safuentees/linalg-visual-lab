#pragma once

#include <array>

#include "math/Types.hpp"

namespace app {

    // Object rotation/translation parameters
    struct TransformParams {
        float yaw = 0.f;
        float pitch = 0.f;
        float pitchPlane = 0.f;
        float axisAngle = 0.f;
        float yTrans = 0.f;
        float distance = 0.f;  // was ws_ (world-space depth)
    };

    // Camera projection parameters
    struct ViewParams {
        float fovDeg = 40.f;
        float focalLength = 1.f;
        bool useCustomLookAt = false;
        bool useParallelProj = false;
        float orthoSize = 5.f;
    };

    // Input sensitivity (rarely changed)
    struct ControlSettings {
        float turnSpeed = 1.f;
        float focalSpeed = 30.f;
    };

    // 3D scene geometry
    struct SceneGeometry {
        std::array<Vec3, 4> grid{};
        std::array<Vec3, 3> vBasis{};
        std::array<Vec3, 3> uBasis{};
        Vec3 a{}, b{}, w{};
        Vec3 originWorld{};
        Vec3 p1{};
        Mat4 arcBall_t{1.f};
        Vec3 last_axis{};
        float last_angle{};
        float dt{};
        float beg_dt{};
        float angular_speed{};
        float end_dt{};

        Vec3 lightPos{};

        bool isDragging{false};
    };
} // namespace app
