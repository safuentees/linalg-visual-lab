//
// Created by Santiago Fuentes on 1/26/26.
//

#ifndef PROJECTION_3D_2D_QUATERNION_H
#define PROJECTION_3D_2D_QUATERNION_H
#include "Types.hpp"

namespace math {
    struct Quat {
        float w, x, y, z;
    };

    Quat fromAxisAngle(const Vec3& axis, float theta);

    inline Quat operator/(const Quat& q, float s) {
        return {q.w / s, q.x / s, q.y / s, q.z / s};
    }

    Mat4 quatToMat4(const Quat& q);
    Quat multiply(const Quat& q1, const Quat& q2);
    Quat normalize(const Quat& q);
    Quat conjugate(const Quat& q);
    float norm(const Quat& q);
}

#endif //PROJECTION_3D_2D_QUATERNION_H