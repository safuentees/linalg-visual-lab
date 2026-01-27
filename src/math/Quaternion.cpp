//
// Created by Santiago Fuentes on 1/26/26.
//
#include "Quaternion.h"
#include <cmath>

namespace math {

    Quat fromAxisAngle(const Vec3& axis, const float theta) {
        const float len = norm({0, axis.x, axis.y, axis.z});
        if (len < 1e-8f) return {1, 0, 0, 0};

        const Vec3 n = {axis.x / len, axis.y / len, axis.z / len};
        const float half = theta * 0.5f;
        const float s = sin(half);

        return { cos(half), s * n.x, s * n.y, s * n.z };
    }

    Mat4 quatToMat4(const Quat& q) { // R[c][r]
        Mat4 R(1.0f);
        R[0][0] = 1 - 2*(q.y*q.y + q.z*q.z);
        R[0][1] = 2*(q.x*q.y + q.w*q.z);
        R[0][2] = 2*(q.x*q.z - q.w*q.y);
        R[1][0] = 2*(q.x*q.y - q.w*q.z);
        R[1][1] = 1 - 2*(q.x*q.x + q.z*q.z);
        R[1][2] = 2*(q.y*q.z + q.w*q.x);
        R[2][0] = 2*(q.x*q.z + q.w*q.y);
        R[2][1] = 2*(q.y*q.z - q.w*q.x);
        R[2][2] = 1 - 2*(q.x*q.x + q.y*q.y);

        return R;
    }

    Quat multiply(const Quat& q1, const Quat& q2) {
        return {
            q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
            q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
            q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
            q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w
        };
    }

    Quat normalize(const Quat& q) {
        const float n = norm(q);
        if (n < 1e-8f) return {1, 0, 0, 0};  // Return identity
        return q / n;
    }

    Quat conjugate(const Quat& q) {
        return { q.w, -q.x, -q.y, -q.z };
    }

    float norm(const Quat& q) {
        return sqrt(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
    }
} // namespace math
