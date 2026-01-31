#include "Types.hpp"
#include "Shadow.h"
#include <glm/ext/matrix_transform.hpp>
namespace math {
    Mat4 shadowFrom(const Vec3 lightPos) {
        const Mat4 T = glm::translate(Mat4(1.f), -lightPos);
        Mat4 M = {1.f};
        M[3][3] = 0.f;
        M[1][3] = 1.0f / -lightPos.y;
        const Mat4 T_back = glm::translate(Mat4(1.f), lightPos);

        return T_back * M * T;
    }
}