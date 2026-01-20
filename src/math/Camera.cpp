#include "math/Camera.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace math {

Vec3 OrbitCamera::Position() const {
    Vec3 pos;
    pos.x = target.x + radius * std::cos(pitch) * std::sin(yaw);
    pos.y = target.y + radius * std::sin(pitch);
    pos.z = target.z + radius * std::cos(pitch) * std::cos(yaw);
    return pos;
}

Mat4 OrbitCamera::ViewMatrix() const {
    return glm::lookAt(Position(), target, up);
}

} // namespace math
