#include "render/Projection.hpp"

#include <cmath>

namespace render {

sf::Vector2f NdcToScreen(const sf::Vector2f& ndc, unsigned int width, unsigned int height) {
    return {
        (ndc.x + 1.f) * 0.5f * width,
        (1.f - (ndc.y + 1.f) * 0.5f) * height
    };
}

sf::Vector2f ToScreenH(const Vec3& world,
                       const Mat4& P,
                       const Mat4& MV,
                       unsigned int width,
                       unsigned int height) {
    Vec4 clip = P * MV * Vec4(world, 1.f);

    // Behind camera or invalid.
    if (std::abs(clip.w) < 1e-6f) {
        return {-99999.f, -99999.f};
    }

    Vec3 ndc = Vec3(clip) / clip.w; // perspective divide
    return NdcToScreen({ndc.x, ndc.y}, width, height);
}

bool ToScreenH(const Vec3& world,
               const Mat4& P,
               const Mat4& MV,
               unsigned int width,
               unsigned int height,
               sf::Vector2f& outScreen) {
    Vec4 clip = P * MV * Vec4(world, 1.f);

    // If w <= 0, point is on/behind the camera plane in the usual convention.
    if (clip.w <= 1e-6f) {
        return false;
    }

    // Clip-space test: -w <= x,y,z <= w.
    if (std::abs(clip.x) > clip.w) {
        return false;
    }
    if (std::abs(clip.y) > clip.w) {
        return false;
    }
    if (std::abs(clip.z) > clip.w) {
        return false;
    }

    Vec3 ndc = Vec3(clip) / clip.w; // perspective divide
    outScreen = NdcToScreen({ndc.x, ndc.y}, width, height);
    return true;
}

} // namespace render
