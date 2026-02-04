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

    Vec3 phong(const Vec3 n, const Vec3 l, const Vec3 v, Vec3 materialColor, float ka, const float kd, const float ks, const float a, Vec3 lightColor) {
        Vec3 Ia = ka * materialColor * lightColor;
        Vec3 Id = kd * glm::max(glm::dot(l, n), 0.f) * materialColor * lightColor;
        const Vec3 r = glm::reflect(-l, n);
        const Vec3 Is = ks * std::pow(glm::max(glm::dot(v, r), 0.f), a)  * lightColor;

        return Ia + Id + Is;
    }


}
