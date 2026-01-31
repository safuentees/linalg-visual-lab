#pragma once

#include "app/SceneParams.hpp"
#include "math/Types.hpp"

namespace ui {

// Frame-specific computed data (calculated each frame during render)
struct FrameContext {
    Mat4 modelView{};
    Mat4 projection{};
    float sceneScale{};
    float aspect{};
    unsigned int windowW{};
    unsigned int windowH{};
};

void ShowMatrixLab(app::TransformParams& transform,
                   app::ViewParams& view,
                   const app::SceneGeometry& scene,
                   const FrameContext& frame);

} // namespace ui