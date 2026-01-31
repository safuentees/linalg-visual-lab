#ifndef PROJECTION_3D_2D_LIGHTING_H
#define PROJECTION_3D_2D_LIGHTING_H
#include "Types.hpp"

namespace math {
    Vec3 faceNormal(const std::array<Vec3,8>& vertices,
                    const std::array<int,4>& face, Mat4 model);

    float phong(Vec3 n, Vec3 l, Vec3 v, float ka, float kd, float ks, float a, float La, float Ld, float Ls);
}
#endif //PROJECTION_3D_2D_LIGHTING_H