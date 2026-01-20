#include "ui.h"

#include <array>
#include <cmath>

static void ImGuiVec3Row(const char* label, const Vec3& v) {
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

static void ImGuiVec4Row(const char* label, const Vec4& v) {
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

static void ImGuiMat4Table(const char* label, const Mat4& m) {
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

static void ImGuiVec3Table(const char* label,
                           const std::array<std::pair<const char*, Vec3>, 7>& rows) {
    ImGui::TextUnformatted(label);
    if (ImGui::BeginTable(label, 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("vec");
        ImGui::TableSetupColumn("x");
        ImGui::TableSetupColumn("y");
        ImGui::TableSetupColumn("z");
        ImGui::TableHeadersRow();
        for (const auto& row : rows) {
            ImGuiVec3Row(row.first, row.second);
        }
        ImGui::EndTable();
    }
}

void ShowMatrixLab(const UiState& state) {
    ImGui::Begin("Matrix Lab");

    if (ImGui::CollapsingHeader("Controls and Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("A/D: focal length  W/S: distance  Arrows: yaw/pitch  Q/E: FOV");
        ImGui::Text("ws: %.3f", state.ws);
        ImGui::Text("yaw: %.3f", state.yaw);
        ImGui::Text("pitch: %.3f", state.pitch);
        ImGui::Text("fov: %.3f", state.fovDeg);
        ImGui::Text("f: %.3f", state.f);
    }

    if (ImGui::CollapsingHeader("Basis and Coordinates", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::array<std::pair<const char*, Vec3>, 7> vRows = {{
            {"v1", state.v1}, {"v2", state.v2}, {"v3", state.v3},
            {"a (coords)", state.a}, {"w (world)", state.w},
            {"u1", state.u1}, {"u2", state.u2}
        }};
        ImGuiVec3Table("v-basis + a, w", vRows);

        std::array<std::pair<const char*, Vec3>, 7> uRows = {{
            {"u1", state.u1}, {"u2", state.u2}, {"u3", state.u3},
            {"b (coords)", state.b}, {"v1", state.v1},
            {"v2", state.v2}, {"v3", state.v3}
        }};
        ImGuiVec3Table("u-basis + b", uRows);
    }

    if (ImGui::CollapsingHeader("Matrices (row view of column-major)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("sceneScale: %.4f  aspect: %.4f", state.sceneScale, state.aspect);
        ImGuiMat4Table("MV_base = T * Rx * Ry", state.MV_base);
        ImGuiMat4Table("MV_vectors = MV_base * S", state.MV_vectors);
        ImGuiMat4Table("P (perspective)", state.P);
    }

    if (ImGui::CollapsingHeader("Pipeline (world -> view -> clip -> ndc -> screen)", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int pointIndex = 0;
        static const char* pointNames[] = {"v1", "v2", "v3", "u1", "u2", "u3", "w"};
        ImGui::Combo("World point", &pointIndex, pointNames, IM_ARRAYSIZE(pointNames));

        const Vec3 world = state.tipVecs[pointIndex];
        const Vec4 view = state.MV_vectors * Vec4(world, 1.f);
        const Vec4 clip = state.P * view;
        const bool validW = std::abs(clip.w) > 1e-6f;
        const Vec3 ndc = validW ? Vec3(clip) / clip.w : Vec3(0.f);
        const sf::Vector2f screen = validW
            ? sf::Vector2f{
                (ndc.x + 1.f) * 0.5f * state.windowW,
                (1.f - (ndc.y + 1.f) * 0.5f) * state.windowH}
            : sf::Vector2f{-99999.f, -99999.f};
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
            ImGuiVec3Row("world", world);
            ImGuiVec4Row("view", view);
            ImGuiVec4Row("clip", clip);
            ImGuiVec3Row("ndc", ndc);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("screen");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f", screen.x);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.1f", screen.y);
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted("-");
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted("-");
            ImGui::EndTable();
        }

        ImGui::Text("clip test: %s", insideClip ? "inside" : "outside");
    }

    ImGui::End();
}
