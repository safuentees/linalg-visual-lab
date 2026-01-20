#pragma once

#include <array>
#include <utility>

#include "math/Types.hpp"

namespace render {

struct CubeMesh {
    std::array<Vec3, 8> vertices{};
    std::array<std::array<int, 4>, 6> faces{};
    std::array<std::pair<int, int>, 12> edges{};
};

CubeMesh MakeCube(float halfSize);

} // namespace render
