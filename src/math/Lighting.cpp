#include "Lighting.h"

namespace math {
    Vec3 faceNormal(const std::array<Vec3,8>& vertices, const std::array<int,4>& face, Mat4 model) {
        const Vec3 v0 = vertices[face[0]];
        const Vec3 v1 = vertices[face[1]];
        const Vec3 v3 = vertices[face[3]];

        const Vec3 edge1 = v1 - v0;
        const Vec3 edge2 = v3 - v0;

        const Vec3 localCenter = (v0 + v1 + vertices[face[2]] + v3) * 0.25f;
        Vec3 normal = glm::normalize(glm::cross(edge1,edge2));
        if (glm::dot(normal, localCenter) < 0.f)
            normal = -normal;

        const auto worldNormal = Vec3(model * Vec4(normal, 0.0f));

        return glm::normalize(worldNormal);
    }

    float phong(Vec3 n, Vec3 l, Vec3 v, float ka, float kd, float ks, float a, float La, float Ld, float Ls) {
        const float Ia = ka * La;
        const float Id = kd * glm::max(glm::dot(l, n), 0.f) * Ld;
        const Vec3 r = glm::reflect(-l, n);
        const float Is = ks * std::pow(glm::max(glm::dot(v, r), 0.f), a)  * Ls;

        return Ia + Id + Is;
    }


}
