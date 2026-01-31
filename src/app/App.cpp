#include "app/App.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <ranges>
#include <tuple>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include "math/Basis.hpp"
#include "math/Lighting.h"
#include "render/Projection.hpp"
#include "ui/MatrixLabUI.hpp"

#include "math/Quaternion.h"
#include "math/Shadow.h"

namespace {
    sf::RenderWindow CreateWindow(unsigned int& outW, unsigned int& outH) {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        constexpr unsigned int kMinWindowSize = 800;
        constexpr unsigned int kMarginW = 80;
        constexpr unsigned int kMarginH = 120;

        outW = std::max(kMinWindowSize,
                        desktop.size.x > kMarginW ? desktop.size.x - kMarginW : desktop.size.x);
        outH = std::max(kMinWindowSize,
                        desktop.size.y > kMarginH ? desktop.size.y - kMarginH : desktop.size.y);

        return {
            sf::VideoMode({outW, outH}),
            "SFML window",
            sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize};
    }

    void AddVectorLine(sf::VertexArray& va,
                       const Vec3& originWorld,
                       const Vec3& vec,
                       const Mat4& P,
                       const Mat4& MV,
                       unsigned int width,
                       unsigned int height,
                       sf::Color color) {
        Vec3 headWorld = originWorld + vec;

        sf::Vector2f origin = render::ToScreenH(originWorld, P, MV, width, height);
        sf::Vector2f head = render::ToScreenH(headWorld, P, MV, width, height);

        std::size_t base = va.getVertexCount();
        va.resize(base + 2);
        va[base + 0].position = origin;
        va[base + 1].position = head;
        va[base + 0].color = color;
        va[base + 1].color = color;
    }

    Mat4 BuildAxisRotation(const Vec3& axis,
                           float theta,
                           Vec3* out_wx,
                           Vec3* out_wz) {
        // “Align to z”: multiply by A (encodes rotations about x, y to get (0, 0, 1))
        // “Rotate about z”: multiply by R_z
        // “Undo alignment”: multiply by A^{-1} (inverse rotations about x, y to get values back)

        const float axisLen = glm::length(axis);
        if (axisLen <= 1e-6f) {
            if (out_wx) {
                *out_wx = Vec3(0.f);
            }
            if (out_wz) {
                *out_wz = Vec3(0.f);
            }
            return {1.f};
        }

        const Vec3 m_w = axis / axisLen;
        const float d = std::sqrt(m_w.y * m_w.y + m_w.z * m_w.z);

        Mat4 Rx(1.f);
        Vec3 w_x = m_w;
        if (d > 1e-6f) {
            const float c = m_w.z / d;
            const float s = m_w.y / d;
            Rx[1][1] = c;
            Rx[2][1] = -s;
            Rx[1][2] = s;
            Rx[2][2] = c;
            w_x = Vec3(Rx * Vec4(m_w, 0.f));
        }
        // atan2 same as approach above longer but more intuitive. Rx to (x,0,z) Ry to (0,0,z).
        const float theta_y = std::atan2(-w_x.x, w_x.z);
        const Mat4 Ry = glm::rotate(Mat4(1.f), theta_y, Vec3{0.f, 1.f, 0.f});
        const Vec3 w_z = Vec3(Ry * Vec4(w_x, 0.f));

        const Mat4 A = Ry * Rx;
        const Mat4 Rz = glm::rotate(Mat4(1.f), theta, Vec3{0.f, 0.f, 1.f});
        const Mat4 R = glm::transpose(A) * Rz * A;

        if (out_wx) {
            *out_wx = w_x;
        }
        if (out_wz) {
            *out_wz = w_z;
        }

        return R;
    }

    Mat4 BuildAxisRotationQuat(const Vec3& axis, const float theta) {
        const math::Quat q = math::fromAxisAngle(axis, theta);
        return math::quatToMat4(q);
    }

    sf::VertexArray BuildWireframe(const render::CubeMesh& cube_,
                                   const Mat4& P,
                                   const Mat4& MV_cube,
                                   unsigned int windowW_,
                                   unsigned int windowH_) {
        sf::VertexArray wire(sf::PrimitiveType::Lines);
        wire.resize(cube_.edges.size() * 2);

        for (std::size_t e = 0; e < cube_.edges.size(); ++e) {
            auto [aIdx, bIdx] = cube_.edges[e];
            sf::Vector2f A;
            sf::Vector2f B;
              if (render::ToScreenH(cube_.vertices[aIdx], P, MV_cube, windowW_, windowH_, A) &&
                render::ToScreenH(cube_.vertices[bIdx], P, MV_cube, windowW_, windowH_, B)) {
                wire[2 * e + 0].position = A;
                wire[2 * e + 1].position = B;
            } else {
                wire[2 * e + 0].position = {-99999.f, -99999.f};
                wire[2 * e + 1].position = {-99999.f, -99999.f};
            }
        }

        return wire;
    }

    sf::VertexArray BuildVectorLines(const std::array<Vec3, 3>& vBasis,
                                     const std::array<Vec3, 3>& uBasis,
                                     const Vec3& w_,
                                     const Mat4& P,
                                     const Mat4& MV_cube,
                                     unsigned int windowW_,
                                     unsigned int windowH_,
                                     const Mat4& MV_plane,
                                     float axisAngle) {
        sf::VertexArray vecLines(sf::PrimitiveType::Lines);

        const auto m_w = w_ / glm::length(w_);

        constexpr Vec3 o = {0, 0, 0};
        AddVectorLine(vecLines, o, m_w, P, MV_cube, windowW_, windowH_, sf::Color::Blue);

        Vec3 w_x(0.f);
        Vec3 w_z(0.f);
        Mat4 R = BuildAxisRotation(w_, axisAngle, &w_x, &w_z);

        Vec3 test = Vec3{0, 1, 0};
        if (glm::length(glm::cross(test, m_w)) < 1e-6f) {
            test = Vec3{1, 0, 0};
        }

        auto rotated = Vec3(R * Vec4(test, 0.f));

        AddVectorLine(vecLines, o, w_x, P, MV_plane, windowW_, windowH_, sf::Color::Green);
        AddVectorLine(vecLines, o, w_z, P, MV_plane, windowW_, windowH_, sf::Color::Red);
        AddVectorLine(vecLines, o, test,     P, MV_plane, windowW_, windowH_, sf::Color::Cyan);
        AddVectorLine(vecLines, o, rotated,  P, MV_plane, windowW_, windowH_, sf::Color::Magenta);

        return vecLines;
    }

    sf::VertexArray BuildTips(const std::array<Vec3, 7>& tipVecs,
                              const Mat4& P,
                              const Mat4& MV_plane,
                              unsigned int windowW_,
                              unsigned int windowH_) {
        sf::VertexArray tips(sf::PrimitiveType::Points);
        tips.resize(7);
        for (std::size_t i = 0; i < tipVecs.size(); ++i) {
            tips[i].position = render::ToScreenH(tipVecs[i], P, MV_plane, windowW_, windowH_);
            tips[i].color = sf::Color::White;
        }

        return tips;
    }

    sf::VertexArray BuildFaces(const render::CubeMesh& cube_,
                               const Mat4& P,
                               const Mat4& MV_cube,
                               const Mat4& model,
                               const Vec3& lightPos,
                               const Vec3& cameraPos,
                               const unsigned int windowW_,
                               const unsigned int windowH_) {
        struct FaceDraw {
            int faceIndex;
            float avgZ;
        };

        std::vector<FaceDraw> order;
        order.reserve(cube_.faces.size());

        for (std::size_t faceIdx = 0; faceIdx < cube_.faces.size(); ++faceIdx) {
            const auto& quad = cube_.faces[faceIdx];

            float zsum = 0.f;
            for (int k = 0; k < 4; ++k) {
                int vidx = quad[k];
                Vec4 vView = MV_cube * Vec4(cube_.vertices[vidx], 1.f);
                zsum += vView.z;
            }
            order.push_back({static_cast<int>(faceIdx), zsum / 4.f});
        }

        std::ranges::sort(order,
                          [](const FaceDraw& a, const FaceDraw& b) { return a.avgZ < b.avgZ; });

        sf::VertexArray faces(sf::PrimitiveType::Triangles);

        for (const auto& item : order) {
            const auto& quad = cube_.faces[item.faceIndex];

            Vec3 normal = math::faceNormal(cube_.vertices, quad, model);
            Vec3 center = (cube_.vertices[quad[0]] + cube_.vertices[quad[1]] + cube_.vertices[quad[2]] + cube_.vertices[quad[3]]) * 0.25f;
            Vec3 worldCenter = Vec3(model * Vec4(center, 1.0f));
            Vec3 l = glm::normalize(lightPos - worldCenter); // from world center to light pos
            Vec3 v = glm::normalize(cameraPos - worldCenter); // from world center to camera pos

            float brightness = math::phong(normal, l, v, 0.1f,
              0.7f, 0.5f, 32.f, 1.f, 1.f, 1.f);

            std::size_t base = faces.getVertexCount();
            faces.resize(base + 6);

            auto c = static_cast<uint8_t>(glm::clamp(brightness, 0.f,1.f) * 255.f);
            sf::Color col = sf::Color(c, c, c);

            static constexpr int triPattern[6] = {0, 1, 2, 0, 2, 3};
            for (int k = 0; k < 6; ++k) {
                int vidx = quad[triPattern[k]];
                sf::Vector2f p = render::ToScreenH(cube_.vertices[vidx], P, MV_cube, windowW_, windowH_);
                faces[base + k].position = p;
                faces[base + k].color = col;
            }
        }

        return faces;
    }

    sf::VertexArray BuildGridLines(const std::array<Vec3, 4>& grid_,
                                   const Mat4& P,
                                   const Mat4& MV_plane,
                                   unsigned int windowW_,
                                   unsigned int windowH_) {
        sf::VertexArray grid(sf::PrimitiveType::Lines, grid_.size());
        for (std::size_t i = 0; i < grid_.size(); ++i) {
            grid[i].position = render::ToScreenH(grid_[i], P, MV_plane, windowW_, windowH_);
        }

        return grid;
    }

    struct GridDrawData {
        std::vector<sf::VertexArray> pairs;
        sf::VertexArray points_grid; // from interpolation
        sf::VertexArray lines_grid; // ignore
    };

    GridDrawData BuildGridDrawData(const std::array<Vec3, 4>& grid_,
                                   const Mat4& P,
                                   const Mat4& MV_plane,
                                   unsigned int windowW_,
                                   unsigned int windowH_) {
        GridDrawData data;
        data.points_grid = sf::VertexArray(sf::PrimitiveType::Points);
        data.points_grid.clear();

        constexpr int N { 10 };
        std::map<std::tuple<int, int>, sf::Vector2f> quad_pos;

        Vec3 A = grid_[0];
        Vec3 B = grid_[1];
        Vec3 C = grid_[2];
        Vec3 D = grid_[3];

        for (int i = 0; i <= N; ++i) {
            for (int j = 0; j <= N; ++j) {
                float u = static_cast<float>(i) / static_cast<float>(N);
                float v = static_cast<float>(j) / static_cast<float>(N);
                // Bilinear interpolation!
                Vec3 P3 = (1.f - u) * (1.f - v) * A +
                          u * (1.f - v) * B +
                          (1.f - u) * v * C +
                          u * v * D;
                quad_pos[{i, j}] = render::ToScreenH(P3, P, MV_plane, windowW_, windowH_);
            }
        }

        for (const auto& t : quad_pos) {
            auto a = t.first;
            auto b = t.second;
            auto i = std::get<0>(a);
            auto j = std::get<1>(a);

            auto itRight = quad_pos.find({i + 1, j});
            if (itRight != quad_pos.end()) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0].position = b;
                line[1].position = itRight->second;
                data.pairs.emplace_back(line);
            }

            auto itUp = quad_pos.find({i, j + 1});
            if (itUp != quad_pos.end()) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0].position = b;
                line[1].position = itUp->second;
                data.pairs.emplace_back(line);
            }
        }

        std::size_t size_grid = data.points_grid.getVertexCount();
        data.lines_grid = sf::VertexArray(sf::PrimitiveType::Lines);

        for (std::size_t i = 0; i < size_grid ; i++) {
            sf::Vertex v;
            sf::Vertex a;

            v.position = data.points_grid[i].position;
            a.position = data.points_grid[i + 1].position;

            data.lines_grid.append(v);
            data.lines_grid.append(a);
        }

        return data;
    }

    Vec3 MapMouseToArcballVec(const int mouseX, const int mouseY, const unsigned int windowW, const unsigned int windowH) {
        // mouseX = 1080 => mouseX / 1080 => 0.f to 1.0f
        // 1.0f * 2 - 1 => [-1, 1]
        const float x { static_cast<float>(mouseX) / static_cast<float>(windowW) * 2 - 1 };
        const float y { - (static_cast<float>(mouseY) / static_cast<float>(windowH) * 2 - 1 ) };
        constexpr float z {};
        const auto len = sqrt(x*x + y*y);

        Vec3 c = { x, y, z };

        if (len > 1.0f) {
            c.x = c.x / len;
            c.y = c.y / len;
            c.z = 0;
        } else {
            c.z = sqrt(1 - x*x - y*y);
        }

        return c;
    }


} // namespace

namespace app {

App::App()
    : window_(CreateWindow(windowW_, windowH_))
{
    window_.setFramerateLimit(120);
    (void)ImGui::SFML::Init(window_);

    // TransformParams, ViewParams, ControlSettings use struct defaults

    camera_.yaw = glm::radians(0.f);
    camera_.pitch = glm::radians(0.f);
    camera_.radius = 8.f;
    camera_.target = {0.f, 0.f, 0.f};
    camera_.up = {0.f, 1.f, 0.f};

    cube_ = render::MakeCube(0.5f);

    scene_.vBasis[0] = {1.f, 0.f, 0.f};
    scene_.vBasis[1] = {0.f, 1.f, 0.f};
    scene_.vBasis[2] = {0.f, 0.f, 1.f};

    scene_.a = {1.f, 2.f, 3.f};
    scene_.w = {1.f, 1.f, 1.f};

    scene_.uBasis[0] = scene_.vBasis[0];
    scene_.uBasis[1] = scene_.vBasis[0] + scene_.vBasis[1];
    scene_.uBasis[2] = scene_.vBasis[0] + scene_.vBasis[1] + scene_.vBasis[2];

    scene_.b = math::CoordsInBasis(scene_.uBasis[0], scene_.uBasis[1], scene_.uBasis[2], scene_.w);

    scene_.lightPos = {2.f, 4.f, 1.f};

    scene_.originWorld = {0.f, 0.f, 0.f};

    float size = 10.f;
    float half = size * 0.5f;
    scene_.grid = {Vec3{+half, 0.f, -half},
                   Vec3{-half, 0.f, -half},
                   Vec3{+half, 0.f, +half},
                   Vec3{-half, 0.f, +half}};

    scene_.p1 = {0,0,0};
}

int App::Run() {
    while (window_.isOpen()) {
        const float dt = clock_.restart().asSeconds();
        ImGui::SFML::Update(window_, sf::seconds(dt));

        ProcessEvents(dt);
        Update(dt);
        Render();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

void App::ProcessEvents(float dt) {
    while (auto ev = window_.pollEvent()) { // returns pointer
        ImGui::SFML::ProcessEvent(window_, *ev);
        if (ev->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                window_.close();
            }
        } else if (const auto* resized = ev->getIf<sf::Event::Resized>()) {
            windowW_ = std::max<unsigned>(1, resized->size.x);
            windowH_ = std::max<unsigned>(1, resized->size.y);
        }
        if (const auto* mouse = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouse->button == sf::Mouse::Button::Left) {
                const auto pos = MapMouseToArcballVec(mouse->position.x, mouse->position.y, windowW_, windowH_);
                scene_.p1 = pos;
                scene_.angular_speed = 0.f;
                scene_.last_angle = 0.f;  // ← Add this!
                scene_.isDragging = true;
            }
        }
        // 0.009 = 0.3
        // 1 = 0.3 / 0.009
        if (const auto* mouse = ev->getIf<sf::Event::MouseButtonReleased>()) {
            scene_.isDragging = false;
            scene_.angular_speed = scene_.last_angle / dt; // per frame speed
            printf("%f, %f, %f \n",scene_.last_angle, dt, scene_.angular_speed);
        }
        if (const auto* mouse = ev->getIf<sf::Event::MouseMoved>(); mouse && scene_.isDragging) {
            auto p2 = MapMouseToArcballVec(mouse->position.x, mouse->position.y, windowW_, windowH_);
            auto axis = glm::cross(scene_.p1, p2);

            float axisLen = glm::length(axis);
            if (axisLen > 0.0001f) {
                axis = axis / axisLen;  // Normalize the axis

                float dotVal = glm::clamp(glm::dot(scene_.p1, p2), -1.0f, 1.0f);
                const float angle = acos(dotVal);
                Mat4 arc_ball = glm::rotate(Mat4(1.f), angle, axis);
                scene_.arcBall_t = arc_ball * scene_.arcBall_t;
                scene_.last_axis = axis;
                scene_.last_angle = angle;
            }
            scene_.p1 = p2;
        }
    }
}

void App::Update(float dt) {
    UpdateControls(dt);

    if (printed_) {
        printed_ = true;
        std::cout << "Book example: a=[1,2,3] in v-basis\n";
        std::cout << "Computed w = (" << scene_.w.x << "," << scene_.w.y << "," << scene_.w.z << ")\n";
        std::cout << "Computed b in u-basis ~ (" << scene_.b.x << "," << scene_.b.y << "," << scene_.b.z << ")\n";
    }
}

void App::UpdateControls(float dt) {
    // Camera orbit — WASD
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        camera_.yaw -= controls_.turnSpeed * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        camera_.yaw += controls_.turnSpeed * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        camera_.pitch += controls_.turnSpeed * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        camera_.pitch -= controls_.turnSpeed * dt;
    }

    // Arcball momentum
    if (!scene_.isDragging && scene_.angular_speed > 0.0001f) {
        float frame_angle = scene_.angular_speed * dt;
        Mat4 rot = glm::rotate(Mat4(1.f), frame_angle, scene_.last_axis);
        scene_.arcBall_t = rot * scene_.arcBall_t;
        scene_.angular_speed *= 0.9975f;
    }
}

float App::ComputeSceneScale() const {
    std::array<Vec3, 7> vecs = {scene_.vBasis[0], scene_.vBasis[1], scene_.vBasis[2],
                                scene_.uBasis[0], scene_.uBasis[1], scene_.uBasis[2],
                                scene_.w};

    float maxVal = 0.f;
    for (const auto& t : vecs) {
        maxVal = std::max(maxVal, math::MaxAbsComponent(t));
    }

    constexpr float halfBox = 0.5f;
    return (maxVal > 0.f) ? (halfBox / maxVal) : 1.f;
}

void App::Render() {
    const float sceneScale = ComputeSceneScale();
    Mat4 view = camera_.ViewMatrix(view_.useCustomLookAt);

    Mat4 MV_plane = glm::translate(Mat4(1.f), Vec3(0.f, 0.f, -transform_.distance));
    MV_plane = view * MV_plane;

    // Local rotation (pitch, yaw, arcball, axis) composed at origin
    Mat4 rotation = Mat4(1.f);
    rotation = glm::rotate(rotation, transform_.pitch, Vec3(1.f, 0.f, 0.f));
    rotation = glm::rotate(rotation, transform_.yaw, Vec3(0.f, 1.f, 0.f));
    rotation = rotation * scene_.arcBall_t;
    rotation = BuildAxisRotationQuat(scene_.w, transform_.axisAngle) * rotation;

    Mat4 shadow = math::shadowFrom(scene_.lightPos);

    // T * R: rotate at origin, then translate into position
    Mat4 modelCube = glm::translate(Mat4(1.f), Vec3(0.f, transform_.yTrans, -transform_.distance));
    modelCube = modelCube * rotation;
    Mat4 MV_shadow = view * shadow * modelCube;

    Mat4 MV_cube = view * modelCube;

    const float aspect = static_cast<float>(windowW_) / static_cast<float>(windowH_);
    Mat4 P = view_.useParallelProj
        ? math::orthographic(view_.orthoSize, aspect, 0.01f, 100.f)
        : glm::perspective(glm::radians(view_.fovDeg), aspect, 0.01f, 100.f);

    sf::VertexArray wire = BuildWireframe(cube_, P, MV_cube, windowW_, windowH_);

    sf::VertexArray vecLines = BuildVectorLines(scene_.vBasis,
                                                scene_.uBasis,
                                                scene_.w,
                                                P,
                                                MV_cube,
                                                windowW_,
                                                windowH_,
                                                MV_plane,
                                                transform_.axisAngle);

    std::array<Vec3, 7> tipVecs = {scene_.vBasis[0], scene_.vBasis[1], scene_.vBasis[2]};

    sf::VertexArray tips = BuildTips(tipVecs, P, MV_plane, windowW_, windowH_);

    sf::Vertex origin(render::ToScreenH(scene_.originWorld, P, MV_plane, windowW_, windowH_));

    sf::VertexArray faces = BuildFaces(cube_, P, MV_cube, modelCube, {1, 1, 1}, camera_.Position(), windowW_, windowH_);

    sf::VertexArray shadow_faces(sf::PrimitiveType::Triangles);
    {
        static constexpr int tri[6] = {0, 1, 2, 0, 2, 3};
        for (const auto& quad : cube_.faces) {
            std::size_t base = shadow_faces.getVertexCount();
            shadow_faces.resize(base + 6);
            for (int k = 0; k < 6; ++k) {
                int vidx = quad[tri[k]];
                sf::Vector2f p = render::ToScreenH(cube_.vertices[vidx], P, MV_shadow, windowW_, windowH_);
                shadow_faces[base + k].position = p;
                shadow_faces[base + k].color = sf::Color(30, 30, 30);
            }
        }
    }

    sf::VertexArray basis = BuildGridLines(scene_.grid, P, MV_plane, windowW_, windowH_);

    auto [pairs, points_grid, lines_grid] = BuildGridDrawData(scene_.grid, P, MV_plane, windowW_, windowH_);

    ui::FrameContext frame{
        .modelView = MV_plane,
        .projection = P,
        .sceneScale = sceneScale,
        .aspect = aspect,
        .windowW = windowW_,
        .windowH = windowH_
    };
    ui::ShowMatrixLab(transform_, view_, scene_, frame);

    window_.clear();

    for (auto& pair : pairs) {
        window_.draw(pair);
    }

    window_.draw(points_grid);
    window_.draw(lines_grid);
    window_.draw(shadow_faces);

    window_.draw(faces);
    // window_.draw(wire);
    window_.draw(vecLines);
    window_.draw(tips);
    window_.draw(&origin, 1, sf::PrimitiveType::Points);
    ImGui::SFML::Render(window_);
    window_.display();
}

} // namespace app
