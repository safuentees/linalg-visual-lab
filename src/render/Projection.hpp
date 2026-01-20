#pragma once

#include <SFML/Graphics.hpp>

#include "math/Types.hpp"

namespace render {

sf::Vector2f NdcToScreen(const sf::Vector2f& ndc, unsigned int width, unsigned int height);

// Homogeneous projection: world -> clip -> NDC -> screen.
// Returns an offscreen value when the point is invalid.
sf::Vector2f ToScreenH(const Vec3& world,
                       const Mat4& P,
                       const Mat4& MV,
                       unsigned int width,
                       unsigned int height);

bool ToScreenH(const Vec3& world,
               const Mat4& P,
               const Mat4& MV,
               unsigned int width,
               unsigned int height,
               sf::Vector2f& outScreen);

} // namespace render
