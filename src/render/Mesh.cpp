#include "render/Mesh.hpp"

namespace render {

CubeMesh MakeCube(float halfSize) {
    const float zNear = -halfSize;
    const float zFar = halfSize;

    CubeMesh mesh;
    mesh.vertices = {{
        {-halfSize,  halfSize,  zNear}, { halfSize,  halfSize,  zNear},
        { halfSize, -halfSize,  zNear}, {-halfSize, -halfSize,  zNear},
        {-halfSize,  halfSize,  zFar }, { halfSize,  halfSize,  zFar },
        { halfSize, -halfSize,  zFar }, {-halfSize, -halfSize,  zFar }
    }};

    mesh.faces = {{
        {{0, 1, 2, 3}}, // near
        {{4, 5, 6, 7}}, // far
        {{0, 1, 5, 4}}, // top
        {{3, 2, 6, 7}}, // bottom
        {{1, 2, 6, 5}}, // right
        {{0, 3, 7, 4}}  // left
    }};

    mesh.edges = {{
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    }};

    return mesh;
}

} // namespace render
