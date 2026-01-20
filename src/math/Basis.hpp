#pragma once

#include "math/Types.hpp"

namespace math {

float MaxAbsComponent(const Vec3& v);

// Compute coordinates c in basis (e1,e2,e3) such that:
// w = c1*e1 + c2*e2 + c3*e3
Vec3 CoordsInBasis(const Vec3& e1,
                   const Vec3& e2,
                   const Vec3& e3,
                   const Vec3& w);

Vec3 FromCoords(const Vec3& e1,
                const Vec3& e2,
                const Vec3& e3,
                const Vec3& c);

} // namespace math
