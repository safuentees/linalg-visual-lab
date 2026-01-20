#include "math/Basis.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace math {

float MaxAbsComponent(const Vec3& v) {
    return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
}

Vec3 CoordsInBasis(const Vec3& e1,
                   const Vec3& e2,
                   const Vec3& e3,
                   const Vec3& w) {
    const Vec3 e2xe3 = glm::cross(e2, e3);
    const Vec3 e3xe1 = glm::cross(e3, e1);
    const Vec3 e1xe2 = glm::cross(e1, e2);

    const float detA = glm::dot(e1, e2xe3);
    if (std::abs(detA) < 1e-6f) {
        return Vec3(0.f); // degenerate basis
    }

    return Vec3(
        glm::dot(w, e2xe3) / detA,
        glm::dot(w, e3xe1) / detA,
        glm::dot(w, e1xe2) / detA
    );
}

Vec3 FromCoords(const Vec3& e1,
                const Vec3& e2,
                const Vec3& e3,
                const Vec3& c) {
    return e1 * c.x + e2 * c.y + e3 * c.z;
}

} // namespace math
