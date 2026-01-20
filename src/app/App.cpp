#include "app/App.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include "math/Basis.hpp"
#include "render/Projection.hpp"
#include "ui/MatrixLabUI.hpp"

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

    return sf::RenderWindow(
        sf::VideoMode({outW, outH}),
        "SFML window",
        sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
}

void AddVectorLine(sf::VertexArray& va,
                   const Vec3& vec,
                   const Mat4& P,
                   const Mat4& MV,
                   unsigned int width,
                   unsigned int height,
                   sf::Color color) {
    Vec3 originWorld{0.f, 0.f, 0.f};
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

} // namespace

namespace app {

App::App() // Members are initialized in the order they are declared in the class, not the order in the initializer list
:     window_(CreateWindow(windowW_, windowH_))
    {
    window_.setFramerateLimit(120);
    ImGui::SFML::Init(window_);
    ws_ = 5.f;
    yTrans_ = 0.f;
    yaw_ = 7.f;
    pitch_ = 7.f;
    pitchPlane_ = 0.f;

    f_ = 1.f;
    fovDeg_ = 40.f;

    turnSpeed_ = 1.f;
    focalSpeed_ = 30.f;

    camera_.yaw = glm::radians(0.f);
    camera_.pitch = glm::radians(0.f);
    camera_.radius = 8.f;
    camera_.target = {0.f, 0.f, 0.f};
    camera_.up = {0.f, 1.f, 0.f};

    cube_ = render::MakeCube(0.5f);

    v1_ = {1.f, 0.f, 0.f};
    v2_ = {0.f, 1.f, 0.f};
    v3_ = {0.f, 0.f, 1.f};

    a_ = {1.f, 2.f, 3.f};
    w_ = math::FromCoords(v1_, v2_, v3_, a_);

    u1_ = v1_;
    u2_ = v1_ + v2_;
    u3_ = v1_ + v2_ + v3_;

    b_ = math::CoordsInBasis(u1_, u2_, u3_, w_);

    originWorld_ = {0.f, 0.f, 0.f};

    float size = 10.f;
    float half = size * 0.5f;
    grid_ = {Vec3{+half, 0.f, -half},
             Vec3{-half, 0.f, -half},
             Vec3{+half, 0.f, +half},
             Vec3{-half, 0.f, +half}};
}

int App::Run() {
    while (window_.isOpen()) {
        const float dt = clock_.restart().asSeconds();
        ImGui::SFML::Update(window_, sf::seconds(dt));

        ProcessEvents();
        Update(dt);
        Render();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

void App::ProcessEvents() {
    while (auto ev = window_.pollEvent()) {
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
    }
}

void App::Update(float dt) {
    UpdateControls(dt);

    if (printed_) {
        printed_ = true;
        std::cout << "Book example: a=[1,2,3] in v-basis\n";
        std::cout << "Computed w = (" << w_.x << "," << w_.y << "," << w_.z << ")\n";
        std::cout << "Computed b in u-basis ~ (" << b_.x << "," << b_.y << "," << b_.z << ")\n";
    }
}

void App::UpdateControls(float dt) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        f_ -= focalSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        f_ += focalSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        ws_ -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        ws_ += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        yaw_ -= turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        yaw_ += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        pitch_ -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        pitch_ += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
        fovDeg_ -= focalSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
        fovDeg_ += focalSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
        yTrans_ -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
        yTrans_ += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
        pitchPlane_ -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G)) {
        pitchPlane_ += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P)) {
        camera_.yaw -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O)) {
        camera_.yaw += turnSpeed_ * dt;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        camera_.pitch -= turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
        camera_.pitch += turnSpeed_ * dt;
    }
}

float App::ComputeSceneScale() const {
    std::array<Vec3, 7> vecs = {v1_, v2_, v3_, u1_, u2_, u3_, w_};

    float maxVal = 0.f;
    for (const auto& t : vecs) {
        maxVal = std::max(maxVal, math::MaxAbsComponent(t));
    }

    const float halfBox = 0.5f;
    return (maxVal > 0.f) ? (halfBox / maxVal) : 1.f;
}

void App::Render() {
    const float sceneScale = ComputeSceneScale();

    Mat4 view = camera_.ViewMatrix();

    Mat4 MV_plane = glm::translate(Mat4(1.f), Vec3(0.f, yTrans_, -ws_));
    MV_plane = view * MV_plane;

    Mat4 modelCube = glm::translate(Mat4(1.f), Vec3(0.f, yTrans_, -ws_));
    modelCube = glm::rotate(modelCube, pitch_, Vec3(1.f, 0.f, 0.f));
    modelCube = glm::rotate(modelCube, yaw_, Vec3(0.f, 1.f, 0.f));

    Mat4 MV_cube = view * modelCube;

    const float aspect = static_cast<float>(windowW_) / static_cast<float>(windowH_);
    Mat4 P = glm::perspective(glm::radians(fovDeg_), aspect, 0.01f, 100.f);

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

    sf::VertexArray vecLines(sf::PrimitiveType::Lines);
    AddVectorLine(vecLines, v1_, P, MV_cube, windowW_, windowH_, sf::Color::Red);
    AddVectorLine(vecLines, v2_, P, MV_cube, windowW_, windowH_, sf::Color::Green);
    AddVectorLine(vecLines, v3_, P, MV_cube, windowW_, windowH_, sf::Color::Blue);

    AddVectorLine(vecLines, u1_, P, MV_cube, windowW_, windowH_, sf::Color(255, 140, 140));
    AddVectorLine(vecLines, u2_, P, MV_cube, windowW_, windowH_, sf::Color(140, 255, 140));
    AddVectorLine(vecLines, u3_, P, MV_cube, windowW_, windowH_, sf::Color(140, 140, 255));

    AddVectorLine(vecLines, w_, P, MV_cube, windowW_, windowH_, sf::Color::White);

    sf::VertexArray tips(sf::PrimitiveType::Points);
    tips.resize(7);
    std::array<Vec3, 7> tipVecs = {v1_, v2_, v3_, u1_, u2_, u3_, w_};
    for (std::size_t i = 0; i < tipVecs.size(); ++i) {
        tips[i].position = render::ToScreenH(tipVecs[i], P, MV_plane, windowW_, windowH_);
        tips[i].color = sf::Color::White;
    }

    sf::Vertex origin(render::ToScreenH(originWorld_, P, MV_plane, windowW_, windowH_));

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

        sf::Color col = sf::Color(80 + item.faceIndex * 20, 140 + item.faceIndex * 20, 220);

        std::size_t base = faces.getVertexCount();
        faces.resize(base + 6);

        static constexpr int triPattern[6] = {0, 1, 2, 0, 2, 3};
        for (int k = 0; k < 6; ++k) {
            int vidx = quad[triPattern[k]];
            sf::Vector2f p = render::ToScreenH(cube_.vertices[vidx], P, MV_cube, windowW_, windowH_);
            faces[base + k].position = p;
            faces[base + k].color = col;
        }
    }

    sf::VertexArray grid(sf::PrimitiveType::Lines, grid_.size());
    for (std::size_t i = 0; i < grid_.size(); ++i) {
        grid[i].position = render::ToScreenH(grid_[i], P, MV_plane, windowW_, windowH_);
    }

    // Pre fix:
    // sf::VertexArray points_grid(sf::PrimitiveType::Points, 72);
    // static constexpr int triPattern[6] = {0, 1, 2, 1, 2, 3};
    // for (int k = 0; k < 6; k += 3) {
    //     Vec3 point = grid_[triPattern[k]];
    //     Vec3 point1 = grid_[triPattern[k + 1]];
    //     Vec3 point2 = grid_[triPattern[k + 2]];
    //
    //     std::vector<glm::vec3> g_points_3d = barycentricGrid(point, point1, point2, 7);
    //     int count { 0 };
    //     for (Vec3 p : g_points_3d) {
    //         sf::Vector2f grid_point = render::ToScreenH(point, P, MV_plane, windowW_, windowH_);
    //         points_grid[count].position = grid_point;
    //         count++;
    //     }
    // }

    sf::VertexArray points_grid(sf::PrimitiveType::Points);
    points_grid.clear();

    static constexpr int triPattern[6] = {0, 1, 2,  1, 2, 3};

    for (int base = 0; base < 6; base += 3) {
        Vec3 A = grid_[triPattern[base + 0]];
        Vec3 B = grid_[triPattern[base + 1]];
        Vec3 C = grid_[triPattern[base + 2]];

        std::vector<Vec3> pts3d = barycentricGrid(A, B, C, 60);

        for (const Vec3& p : pts3d) {
            sf::Vertex v;
            v.position = render::ToScreenH(p, P, MV_plane, windowW_, windowH_);
            v.color = sf::Color::White; // optional
            points_grid.append(v);
        }
    }

    std::size_t size_grid = points_grid.getVertexCount();
    sf::VertexArray lines_grid(sf::PrimitiveType::Lines);
    for (std::size_t i = 0; i < size_grid ; i++) {
        sf::Vertex v;
        sf::Vertex a;

        v.position = points_grid[i].position;
        a.position = points_grid[i + 1].position;

        lines_grid.append(v);
        lines_grid.append(a);
    }
    // 0 1 1 2 2 3 3 4 4 5 5 6

    UiState uiState{
        .ws = ws_,
        .yaw = yaw_,
        .pitch = pitch_,
        .fovDeg = fovDeg_,
        .f = f_,
        .sceneScale = sceneScale,
        .aspect = aspect,
        .windowW = windowW_,
        .windowH = windowH_,
        .v1 = v1_,
        .v2 = v2_,
        .v3 = v3_,
        .u1 = u1_,
        .u2 = u2_,
        .u3 = u3_,
        .a = a_,
        .b = b_,
        .w = w_,
        .MV_base = MV_plane,
        .MV_vectors = MV_plane,
        .P = P,
        .tipVecs = tipVecs
    };
    ShowMatrixLab(uiState);

    window_.clear();
    window_.draw(points_grid);
    window_.draw(lines_grid);
    window_.draw(faces);
    window_.draw(wire);
    window_.draw(vecLines);
    window_.draw(tips);
    window_.draw(&origin, 1, sf::PrimitiveType::Points);
    ImGui::SFML::Render(window_);
    window_.display();
}

std::vector<glm::vec3> App::barycentricGrid(const glm::vec3& A,
                                       const glm::vec3& B,
                                       const glm::vec3& C,
                                       int N)
{
    std::vector<glm::vec3> pts;
    if (N <= 0) {                // avoid divide-by-zero
        pts.push_back(A);        // or centroid, your choice
        return pts;
    }

    pts.reserve((N + 1) * (N + 2) / 2);

    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            // 0 1 2 3(=).
            // k = 3 - 0 - 0, 2, 1, 0
            // j = 0, 0, 0, 0
            // (0, 0, 3)
            int k = N - i - j;

            float u = float(i) / float(N);
            float v = float(j) / float(N);
            float w = float(k) / float(N);   // u+v+w == 1

            glm::vec3 P = u*A + v*B + w*C;
            pts.push_back(P);
        }
    }
    return pts;
}

} // namespace app
