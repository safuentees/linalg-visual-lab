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

    Mat4 OrbitCamera::ViewMatrix(bool useCustom) const {
        return useCustom
            ? lookAtMatrix(Position(), target, up)
            : glm::lookAt(Position(), target, up);
    }

    Mat4 lookAtMatrix(Vec3 pos, Vec3 target, Vec3 up) {
        Vec3 n = glm::normalize(target - pos); // Vector direction.
        Vec3 right = glm::normalize(glm::cross(n, up));
        Vec3 true_up = glm::normalize(glm::cross(right, n));

        Mat4 R = { 1.f };
        R[0][0] = right.x;
        R[1][0] = right.y;
        R[2][0] = right.z;
        R[0][1] = true_up.x;
        R[1][1] = true_up.y;
        R[2][1] = true_up.z;
        R[0][2] = -n.x;
        R[1][2] = -n.y;
        R[2][2] = -n.z;

        Mat4 T = { 1.f };
        T[3][0] = -pos.x;
        T[3][1] = -pos.y;
        T[3][2] = -pos.z;

        return R * T;
    }

    Mat4 orthographic(const float& orthoSize, const float& aspect, const float& near, const float& far) {

        const float right = orthoSize * aspect;
        const float top = orthoSize;

        Mat4 P_ortho = {1.f};
        P_ortho[0][0] = 1/right;
        P_ortho[1][1] = 1/top;
        P_ortho[2][2] = -2/(far-near);
        P_ortho[3][2] = -(far+near)/(far-near);

        return P_ortho;
    }
} // namespace math
