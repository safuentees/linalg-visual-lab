#include "ui/MatrixLabUI.hpp"

#include <cmath>

#include <imgui.h>

namespace {

void Vec3Row(const char* label, const Vec3& v) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f", v.x);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.3f", v.y);
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.3f", v.z);
}

void Vec4Row(const char* label, const Vec4& v) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f", v.x);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.3f", v.y);
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.3f", v.z);
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%.3f", v.w);
}

void Mat4Table(const char* label, const Mat4& m) {
    ImGui::TextUnformatted(label);
    ImGui::PushID(label);
    if (ImGui::BeginTable("##mat4", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        for (int row = 0; row < 4; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < 4; ++col) {
                ImGui::TableSetColumnIndex(col);
                ImGui::Text("%.4f", m[col][row]);
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void ModeTogglesSection(app::ViewParams& view) {
    if (!ImGui::CollapsingHeader("Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::Checkbox("Custom LookAt", &view.useCustomLookAt);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", view.useCustomLookAt ? "yours" : "glm");

    ImGui::Checkbox("Orthographic", &view.useParallelProj);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", view.useParallelProj ? "ortho" : "perspective");
}

void ObjectTransformSection(app::TransformParams& transform) {
    if (!ImGui::CollapsingHeader("Object Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::DragFloat("Distance", &transform.distance, 0.05f);
    ImGui::DragFloat("Yaw", &transform.yaw, 0.01f);
    ImGui::DragFloat("Pitch", &transform.pitch, 0.01f);
    ImGui::DragFloat("Y Translate", &transform.yTrans, 0.05f);
    ImGui::DragFloat("Axis Angle", &transform.axisAngle, 0.01f);
    ImGui::DragFloat("Plane Pitch", &transform.pitchPlane, 0.01f);
    if (ImGui::Button("Reset Transform")) {
        transform = app::TransformParams{};
    }
}

void CameraSection(app::ViewParams& view) {
    if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::TextDisabled("Orbit: W/A/S/D");
    ImGui::DragFloat("FOV", &view.fovDeg, 0.5f);
    ImGui::DragFloat("Focal Length", &view.focalLength, 0.1f);
    if (view.useParallelProj) {
        ImGui::DragFloat("Ortho Size", &view.orthoSize, 0.1f);
    }
}

void BasisSection(const app::SceneGeometry& scene) {
    if (!ImGui::CollapsingHeader("Basis and Coordinates")) {
        return;
    }

    if (ImGui::BeginTable("v-basis", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("vec");
        ImGui::TableSetupColumn("x");
        ImGui::TableSetupColumn("y");
        ImGui::TableSetupColumn("z");
        ImGui::TableHeadersRow();
        Vec3Row("v1", scene.vBasis[0]);
        Vec3Row("v2", scene.vBasis[1]);
        Vec3Row("v3", scene.vBasis[2]);
        Vec3Row("a (coords)", scene.a);
        Vec3Row("w (world)", scene.w);
        ImGui::EndTable();
    }

    if (ImGui::BeginTable("u-basis", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("vec");
        ImGui::TableSetupColumn("x");
        ImGui::TableSetupColumn("y");
        ImGui::TableSetupColumn("z");
        ImGui::TableHeadersRow();
        Vec3Row("u1", scene.uBasis[0]);
        Vec3Row("u2", scene.uBasis[1]);
        Vec3Row("u3", scene.uBasis[2]);
        Vec3Row("b (coords)", scene.b);
        ImGui::EndTable();
    }
}

void MatricesSection(const ui::FrameContext& frame) {
    if (!ImGui::CollapsingHeader("Matrices")) {
        return;
    }
    ImGui::Text("sceneScale: %.4f  aspect: %.4f", frame.sceneScale, frame.aspect);
    Mat4Table("ModelView", frame.modelView);
    Mat4Table("Projection", frame.projection);
}

void PipelineSection(const app::SceneGeometry& scene, const ui::FrameContext& frame) {
    if (!ImGui::CollapsingHeader("Pipeline (world -> screen)")) {
        return;
    }

    static int pointIndex = 0;
    static const char* pointNames[] = {"v1", "v2", "v3", "u1", "u2", "u3", "w"};
    ImGui::Combo("World point", &pointIndex, pointNames, IM_ARRAYSIZE(pointNames));

    const std::array<Vec3, 7> points = {
        scene.vBasis[0], scene.vBasis[1], scene.vBasis[2],
        scene.uBasis[0], scene.uBasis[1], scene.uBasis[2],
        scene.w
    };

    const Vec3 world = points[pointIndex];
    const Vec4 viewCoord = frame.modelView * Vec4(world, 1.f);
    const Vec4 clip = frame.projection * viewCoord;
    const bool validW = std::abs(clip.w) > 1e-6f;
    const Vec3 ndc = validW ? Vec3(clip) / clip.w : Vec3(0.f);
    const float screenX = validW ? (ndc.x + 1.f) * 0.5f * static_cast<float>(frame.windowW) : -99999.f;
    const float screenY = validW ? (1.f - (ndc.y + 1.f) * 0.5f) * static_cast<float>(frame.windowH) : -99999.f;
    const bool insideClip = validW &&
                            std::abs(clip.x) <= clip.w &&
                            std::abs(clip.y) <= clip.w &&
                            std::abs(clip.z) <= clip.w;

    if (ImGui::BeginTable("##pipeline", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("stage");
        ImGui::TableSetupColumn("x");
        ImGui::TableSetupColumn("y");
        ImGui::TableSetupColumn("z");
        ImGui::TableSetupColumn("w");
        ImGui::TableHeadersRow();

        Vec3Row("world", world);
        Vec4Row("view", viewCoord);
        Vec4Row("clip", clip);
        Vec3Row("ndc", ndc);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("screen");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f", screenX);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.1f", screenY);
        ImGui::TableSetColumnIndex(3);
        ImGui::TextUnformatted("-");
        ImGui::TableSetColumnIndex(4);
        ImGui::TextUnformatted("-");
        ImGui::EndTable();
    }

    ImGui::Text("clip test: %s", insideClip ? "inside" : "outside");
}

} // namespace

namespace ui {

void ShowMatrixLab(app::TransformParams& transform,
                   app::ViewParams& view,
                   const app::SceneGeometry& scene,
                   const FrameContext& frame) {
    ImGui::Begin("Matrix Lab");
    ImGui::PushItemWidth(100.f);

    ModeTogglesSection(view);
    ObjectTransformSection(transform);
    CameraSection(view);
    BasisSection(scene);
    MatricesSection(frame);
    PipelineSection(scene, frame);

    ImGui::PopItemWidth();
    ImGui::End();
}

} // namespace ui
