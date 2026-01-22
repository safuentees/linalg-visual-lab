#include "app/App.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <ranges>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include "math/Basis.hpp"
#include "render/Projection.hpp"
#include "ui/MatrixLabUI.hpp"

#include <map>
#include <tuple>
#include <string>

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

void AddVectorLine(sf::VertexArray& va,
                   const Vec3& vec,
                   const Mat4& P,
                   const Mat4& MV,
                   unsigned int width,
                   unsigned int height,
                   sf::Color color) {

    AddVectorLine(va, Vec3{0.f, 0.f, 0.f}, vec, P, MV, width, height, color);
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
        return Mat4(1.f);
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
    // AddVectorLine(vecLines, {1,0,0}, P, MV_plane, windowW_, windowH_, sf::Color::Red);
    // AddVectorLine(vecLines, {0,1,0}, P, MV_plane, windowW_, windowH_, sf::Color::Green);
    // AddVectorLine(vecLines, {0,0,1}, P, MV_plane, windowW_, windowH_, sf::Color::Blue);

    // AddVectorLine(vecLines, uBasis[0], P, MV_cube, windowW_, windowH_, sf::Color(255, 140, 140));
    // AddVectorLine(vecLines, uBasis[1], P, MV_cube, windowW_, windowH_, sf::Color(140, 255, 140));
    // AddVectorLine(vecLines, uBasis[2], P, MV_cube, windowW_, windowH_, sf::Color(140, 140, 255));
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
                           unsigned int windowW_,
                           unsigned int windowH_) {
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

    const int N { 10 };
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

} // namespace

namespace app {

App::App() // Members are initialized in the order they are declared in the class, not the order in the initializer list
:     window_(CreateWindow(windowW_, windowH_))
    {
    window_.setFramerateLimit(120);
    ImGui::SFML::Init(window_);
    ws_ = 0.f;
    yTrans_ = 0.f;
    yaw_ = 0.f;
    pitch_ = 0.f;
    pitchPlane_ = 0.f;
    axisAngle_ = 0.f;

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

    vBasis_[0] = {1.f, 0.f, 0.f};
    vBasis_[1] = {0.f, 1.f, 0.f};
    vBasis_[2] = {0.f, 0.f, 1.f};

    a_ = {1.f, 2.f, 3.f};
    // w_ = math::FromCoords(vBasis_[0], vBasis_[1], vBasis_[2], a_);
    w_ = {1.f, 1.f, 1.f};

    uBasis_[0] = vBasis_[0];
    uBasis_[1] = vBasis_[0] + vBasis_[1];
    uBasis_[2] = vBasis_[0] + vBasis_[1] + vBasis_[2];

    b_ = math::CoordsInBasis(uBasis_[0], uBasis_[1], uBasis_[2], w_);

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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad0)) {
        axisAngle_ += turnSpeed_ * dt;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad9)) {
        axisAngle_ -= turnSpeed_ * dt;
    }
}

float App::ComputeSceneScale() const {
    std::array<Vec3, 7> vecs = {vBasis_[0], vBasis_[1], vBasis_[2],
                                uBasis_[0], uBasis_[1], uBasis_[2],
                                w_};

    float maxVal = 0.f;
    for (const auto& t : vecs) {
        maxVal = std::max(maxVal, math::MaxAbsComponent(t));
    }

    const float halfBox = 0.5f;
    return (maxVal > 0.f) ? (halfBox / maxVal) : 1.f;
}

void App:: Render() {
    const float sceneScale = ComputeSceneScale();
    float tetha_arb = 0.f;
    Mat4 view = camera_.ViewMatrix();

    Mat4 MV_plane = glm::translate(Mat4(1.f), Vec3(0.f, yTrans_, -ws_));
    MV_plane = view * MV_plane;

    Mat4 modelCube = glm::translate(Mat4(1.f), Vec3(0.f, yTrans_, -ws_));
    modelCube = glm::rotate(modelCube, pitch_, Vec3(1.f, 0.f, 0.f));
    modelCube = glm::rotate(modelCube, yaw_, Vec3(0.f, 1.f, 0.f));
    Mat4 axisRot = BuildAxisRotation(w_, axisAngle_, nullptr, nullptr);
    modelCube = axisRot * modelCube;

    Vec3 w_vec{1,1,1};
    auto d = sqrt(w_vec.y * w_vec.y + w_vec.z * w_vec.y);

    Mat4 MV_cube = view * modelCube;



    const float aspect = static_cast<float>(windowW_) / static_cast<float>(windowH_);
    Mat4 P = glm::perspective(glm::radians(fovDeg_), aspect, 0.01f, 100.f);

    sf::VertexArray wire = BuildWireframe(cube_, P, MV_cube, windowW_, windowH_);

    sf::VertexArray vecLines = BuildVectorLines(vBasis_,
                                                uBasis_,
                                                w_,
                                                P,
                                                MV_cube,
                                                windowW_,
                                                windowH_,
                                                MV_plane,
                                                axisAngle_);

    std::array<Vec3, 7> tipVecs = {vBasis_[0], vBasis_[1], vBasis_[2]};
    sf::VertexArray tips = BuildTips(tipVecs, P, MV_plane, windowW_, windowH_);

    sf::Vertex origin(render::ToScreenH(originWorld_, P, MV_plane, windowW_, windowH_));

    sf::VertexArray faces = BuildFaces(cube_, P, MV_cube, windowW_, windowH_);

    sf::VertexArray basis = BuildGridLines(grid_, P, MV_plane, windowW_, windowH_);


    GridDrawData gridDraw = BuildGridDrawData(grid_, P, MV_plane, windowW_, windowH_);

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
        .v1 = vBasis_[0],
        .v2 = vBasis_[1],
        .v3 = vBasis_[2],
        .u1 = uBasis_[0],
        .u2 = uBasis_[1],
        .u3 = uBasis_[2],
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

    for (auto& pair : gridDraw.pairs) {
        window_.draw(pair);
    }

    window_.draw(gridDraw.points_grid);
    window_.draw(gridDraw.lines_grid);
    window_.draw(faces);
    // window_.draw(wire);
    window_.draw(vecLines);
    // window_.draw(tips);
    window_.draw(&origin, 1, sf::PrimitiveType::Points);
    ImGui::SFML::Render(window_);
    window_.display();
}

std::map<std::tuple<int,int>, sf::Vector2f> App:: barycentricGrid(const glm::vec3& A,
                                       const glm::vec3& B,
                                       const glm::vec3& C,
                                       const int N,
                                       const Mat4 &Pr,
                                       const Mat4 &M
                                       ) const
{
    std::vector<glm::vec3> pts;
    if (N <= 0) {                // avoid divide-by-zero
        // TODO: Fix
    }
    pts.reserve((N + 1) * (N + 2) / 2); // ???

    std::map<std::tuple<int,int>, sf::Vector2f> m;

    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            int k = N - i - j;

            float u = float(i) / float(N);
            float v = float(j) / float(N);
            float w = float(k) / float(N);   // u+v+w == 1

            glm::vec3 P = u*A + v*B + w*C;
            const auto r  = render::ToScreenH(P,Pr,M,windowW_,windowH_);
            m[{i, j}] = r;
        }
    }
    return m;
}

} // namespace app
