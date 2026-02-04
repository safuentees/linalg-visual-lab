#ifndef PROJECTION_3D_2D_LIGHTING_H
#define PROJECTION_3D_2D_LIGHTING_H
#include "Types.hpp"

namespace math {
    Vec3 faceNormal(const std::array<Vec3,8>& vertices,
                    const std::array<int,4>& face, Mat4 model);

    Vec3 phong(Vec3 n, Vec3 l, Vec3 v, Vec3 materialColor, float ka, float kd, float ks, float a, Vec3 lightColor);
}
#endif //PROJECTION_3D_2D_LIGHTING_H