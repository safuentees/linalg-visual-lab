#pragma once

#include "math/Types.hpp"

namespace math {

struct OrbitCamera {
    Vec3 target{0.f, 0.f, 0.f};
    Vec3 up{0.f, 1.f, 0.f};
    float yaw{0.f};
    float pitch{0.f};
    float radius{8.f};

    Vec3 Position() const;
    Mat4 ViewMatrix() const;
};

} // namespace math
