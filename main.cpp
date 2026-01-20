#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

#include "ui.h"

using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

static sf::Vector2f ndcToScreen(const sf::Vector2f& ndc, unsigned W, unsigned H) {
    return {
        (ndc.x + 1.f) * 0.5f * W,
        (1.f - (ndc.y + 1.f) * 0.5f) * H
    };
}

// Homogeneous projection: world -> clip -> NDC -> screen
static sf::Vector2f toScreenH(const Vec3& world,
                              const Mat4& P,
                              const Mat4& MV,
                              unsigned W,
                              unsigned H)
{
    Vec4 clip = P * MV * Vec4(world, 1.f);

    // Behind camera or invalid
    if (std::abs(clip.w) < 1e-6f) return {-99999.f, -99999.f};

    Vec3 ndc3 = Vec3(clip) / clip.w;   // perspective divide  [oai_citation:3‡Learn OpenGL](https://learnopengl.com/Getting-started/Coordinate-Systems?utm_source=chatgpt.com)

    // If you want: clip outside NDC range -> hide
    // if (ndc3.x < -1 || ndc3.x > 1 || ndc3.y < -1 || ndc3.y > 1) ...

    return ndcToScreen({ndc3.x, ndc3.y}, W, H);
}

static bool toScreenH(const Vec3& world,
                      const Mat4& P,
                      const Mat4& MV,
                      unsigned W,
                      unsigned H,
                      sf::Vector2f& outScreen)
{
    Vec4 clip = P * MV * Vec4(world, 1.f);

    // If w <= 0, point is on/behind the camera plane in the usual convention.
    if (clip.w <= 1e-6f) return false;

    // Clip-space test: -w <= x,y,z <= w
    // (This is the key GPU-style clipping rule.)  [oai_citation:3‡songho.ca](https://www.songho.ca/opengl/gl_projectionmatrix.html?utm_source=chatgpt.com)
    if (std::abs(clip.x) > clip.w) return false;
    if (std::abs(clip.y) > clip.w) return false;
    if (std::abs(clip.z) > clip.w) return false;

    Vec3 ndc = Vec3(clip) / clip.w; // perspective divide
    outScreen = ndcToScreen({ndc.x, ndc.y}, W, H);
    return true;
}

static float maxAbsComponent(const Vec3& v) {
    return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
}

// Compute coordinates c in basis (e1,e2,e3) such that:
// w = c1*e1 + c2*e2 + c3*e3
static Vec3 coordsInBasis(const Vec3& e1,
                          const Vec3& e2,
                          const Vec3& e3,
                          const Vec3& w)
{
    const Vec3 e2xe3 = glm::cross(e2, e3);
    const Vec3 e3xe1 = glm::cross(e3, e1);
    const Vec3 e1xe2 = glm::cross(e1, e2);

    const float detA = glm::dot(e1, e2xe3);
    if (std::abs(detA) < 1e-6f) {
        return Vec3(0.f); // degenerate basis
    }

    return Vec3(
        glm::dot(w, e2xe3) / detA,
        glm::dot(w, e3xe1) / detA,
        glm::dot(w, e1xe2) / detA
    );
}

static Vec3 fromCoords(const Vec3& e1,
                       const Vec3& e2,
                       const Vec3& e3,
                       const Vec3& c)
{
    return e1 * c.x + e2 * c.y + e3 * c.z;
}

void controls(float& f, float focalSpeed, float dt, float& ws, float turnSpeed, float& yaw, float& pitch, float& fovdeg, float& y_trans, float& p_p,  float& camPitch, float& cy)  {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) f -= focalSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) f += focalSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) ws -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) ws += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  yaw   -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) yaw   += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    pitch -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  pitch += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))    fovdeg -= focalSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))  fovdeg += focalSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z))  y_trans -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X))  y_trans += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F))  p_p -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G))  p_p += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P))  camPitch -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O))  camPitch += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L))  cy -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K))  cy += turnSpeed * dt;

}

int main() {
    float ws    = 5.f;
    float y_trans = 0.f;
    float yaw   = 7.f;
    float pitch = 7.f;
    float pitch_plane = 0.f;


    // Camera (orbit) state (radians)
    float camYaw   = glm::radians(0.f);   // left/right around Y
    float camPitch = glm::radians(0.f);  // up/down
    float camRadius = 8.f;                // distance from target

    Vec3 camTarget {0.f, 0.f, 0.f};
    Vec3 worldUp   {0.f, 1.f, 0.f};


    float f     = 1.f;
    float fovDeg = 40.f; // make this a variable

    sf::Clock clock;
    const float turnSpeed  = 1.f;
    const float focalSpeed = 30.f;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    constexpr unsigned int kMinWindowSize = 800;
    constexpr unsigned int kMarginW = 80;
    constexpr unsigned int kMarginH = 120;
    unsigned int windowW = std::max(kMinWindowSize,
                                    desktop.size.x > kMarginW ? desktop.size.x - kMarginW : desktop.size.x);
    unsigned int windowH = std::max(kMinWindowSize,
                                    desktop.size.y > kMarginH ? desktop.size.y - kMarginH : desktop.size.y);

    sf::RenderWindow window(
        sf::VideoMode({windowW, windowH}),
        "SFML window",
        sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    ImGui::SFML::Init (window);
    // ---------- cube wireframe ----------
    const float zNear = -0.5f;
    const float zFar  =  0.5f;

    const std::array<Vec3, 8> cubeVerts = {{
        {-0.5f,  0.5f,  zNear}, { 0.5f,  0.5f,  zNear},
        { 0.5f, -0.5f,  zNear}, {-0.5f, -0.5f,  zNear},
        {-0.5f,  0.5f,  zFar }, { 0.5f,  0.5f,  zFar },
        { 0.5f, -0.5f,  zFar }, {-0.5f, -0.5f,  zFar }
    }};

    static constexpr std::array<std::array<int,4>, 6> cubeFaces = {{
        {{0,1,2,3}}, // near
        {{4,5,6,7}}, // far
        {{0,1,5,4}}, // top
        {{3,2,6,7}}, // bottom
        {{1,2,6,5}}, // right
        {{0,3,7,4}}  // left
    }};

    constexpr std::array<std::pair<int,int>, 12> cubeEdges = {{
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    }};

    Vec3 v1{1.f, 0.f, 0.f};
    Vec3 v2{0.f, 1.f, 0.f};
    Vec3 v3{0.f, 0.f, 1.f};

    Vec3 a{1.f, 2.f, 3.f};
    Vec3 w = fromCoords(v1, v2, v3, a);

    Vec3 u1 = v1;
    Vec3 u2 = v1 + v2;
    Vec3 u3 = v1 + v2 + v3;

    Vec3 b = coordsInBasis(u1, u2, u3, w);

    Vec3 origin_world {0, 0, 0};
    float S = 10.f;
    float h = S * 0.5f;

    Vec3 p0 = { +h, 0, -h };
    Vec3 p1 = { -h, 0, -h };
    Vec3 p2 = { +h, 0, +h };
    Vec3 p3 = { -h, 0, +h };

    int N  { 0 };

    std::array<Vec3, 4> vecs_grid = { p0, p1, p2, p3 };
    sf::VertexArray grid_points(sf::PrimitiveType::Points, 0);
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            int k = N - i - j;          // forced by the sum constraint
            // barycentric weights:
            grid_points.resize(grid_points.getVertexCount() + 1);
            float a_bar = float(i) / N;
            float b_bar = float(j) / N;
            float c_bar = float(k) / N;     // a + b + c == 1
            emit(a_bar, b_bar,c_bar);
        }
    }

    // const std::array<Vec3, 4> plane = {{
    //     {-0.5f,  0,  zNear}, { 0.5f,  0,  zNear},
    //     { 0.5f, 0,  zNear}, {-0.5f, 0,  zNear},
    //     {-0.5f,  0,  zFar }, { 0.5f,  0,  zFar },
    //     { 0.5f, -0.5,  zFar }, {-0.5f, 0,  zFar }
    // }};
    //w

    auto computeSceneScale = [&]() -> float {
        std::array<Vec3, 7> vecs = { v1, v2, v3, u1, u2, u3, w };

        float m = 0.f;
        for (const auto& t : vecs) m = std::max(m, maxAbsComponent(t));

        const float halfBox = 0.5f;
        return (m > 0.f) ? (halfBox / m) : 1.f;
    };

    bool printed = false;

    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        while (auto ev = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *ev);
            if (ev->is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
            else if (const auto* resized = ev->getIf<sf::Event::Resized>())
            {
                windowW = std::max<unsigned>(1, resized->size.x);
                windowH = std::max<unsigned>(1, resized->size.y);
            }
        }

        controls(f, focalSpeed, dt, ws, turnSpeed, yaw, pitch, fovDeg, y_trans, pitch_plane, camYaw,camPitch);

        const float sceneScale = computeSceneScale();

        // Orbit camera position from spherical coords
        Vec3 camPos;
        camPos.x = camTarget.x + camRadius * std::cos(camPitch) * std::sin(camYaw);
        camPos.y = camTarget.y + camRadius * std::sin(camPitch);
        camPos.z = camTarget.z + camRadius * std::cos(camPitch) * std::cos(camYaw);

        Mat4 View = glm::lookAt(camPos, camTarget, worldUp);

        auto Model_plane = Mat4(1.f); // identity, no model transform
        Mat4 MV_plane = glm::translate(Mat4(1.f), Vec3(0, y_trans, -ws));
        MV_plane = View * Model_plane * MV_plane;

        Mat4 Model_cube = glm::translate(Mat4(1.f), Vec3(0, y_trans, -ws));
        Model_cube = glm::rotate(Model_cube, pitch, Vec3(1,0,0));
        Model_cube = glm::rotate(Model_cube, yaw,   Vec3(0,1,0));

        Mat4 MV_cube = View * Model_cube;

        // ---- Projection ----
        float aspect = float(windowW) / float(windowH);

        Mat4 P = glm::perspective(glm::radians(fovDeg), aspect, 0.01f, 100.f);

        auto addVectorLine = [&](sf::VertexArray& va, const Vec3& vec, sf::Color color)
        {
            Vec3 originWorld{0.f, 0.f, 0.f}; // Translation Vector in Matrix
            Vec3 headWorld   = originWorld + vec;

            sf::Vector2f O = toScreenH(originWorld, P, MV_cube, windowW, windowH);
            sf::Vector2f H = toScreenH(headWorld,   P, MV_cube, windowW, windowH);

            std::size_t base = va.getVertexCount();
            va.resize(base + 2);
            va[base + 0].position = O;
            va[base + 1].position = H;
            va[base + 0].color = color;
            va[base + 1].color = color;
        };

        sf::VertexArray wire(sf::PrimitiveType::Lines);
        wire.resize(cubeEdges.size() * 2);

        for (std::size_t e = 0; e < cubeEdges.size(); ++e) {
            auto [aIdx, bIdx] = cubeEdges[e];
            sf::Vector2f A, B;
            if (toScreenH(cubeVerts[aIdx], P, MV_cube, windowW, windowH, A) &&
                toScreenH(cubeVerts[bIdx], P, MV_cube, windowW, windowH, B))
            {
                wire[2*e+0].position = A;
                wire[2*e+1].position = B;
            }
            else
            {
                // hide this edge if either endpoint clipped (simple, not perfect)
                wire[2*e+0].position = {-99999.f, -99999.f};
                wire[2*e+1].position = {-99999.f, -99999.f};
            }
        }

        sf::VertexArray vecLines(sf::PrimitiveType::Lines);

        addVectorLine(vecLines, v1, sf::Color::Red);
        addVectorLine(vecLines, v2, sf::Color::Green);
        addVectorLine(vecLines, v3, sf::Color::Blue);

        addVectorLine(vecLines, u1, sf::Color(255, 140, 140));
        addVectorLine(vecLines, u2, sf::Color(140, 255, 140));
        addVectorLine(vecLines, u3, sf::Color(140, 140, 255));

        addVectorLine(vecLines, w, sf::Color::White);

        sf::VertexArray tips(sf::PrimitiveType::Points);
        tips.resize(7);
        std::array<Vec3, 7> tipVecs = { v1, v2, v3, u1, u2, u3, w };
        for (std::size_t i = 0; i < tipVecs.size(); ++i) {
            tips[i].position = toScreenH(tipVecs[i], P, MV_plane, windowW, windowH);
            tips[i].color = sf::Color::White;
        }

        sf::Vertex origin(toScreenH(origin_world, P, MV_plane, windowW, windowH));

        if (!printed) {
            printed = true;
            std::cout << "Book example: a=[1,2,3] in v-basis\n";
            std::cout << "Computed w = (" << w.x << "," << w.y << "," << w.z << ")\n";
            std::cout << "Computed b in u-basis ~ (" << b.x << "," << b.y << "," << b.z << ")\n";
        }

        struct FaceDraw {
            int faceIndex;
            float avgZ; // view-space z
        };

        std::vector<FaceDraw> order;
        order.reserve(6);

        for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
            const auto& quad = cubeFaces[faceIdx];

            float zsum = 0.f;
            for (int k = 0; k < 4; ++k) {
                int vidx = quad[k];
                Vec4 vView = MV_cube * Vec4(cubeVerts[vidx], 1.f);
                // from model to view without projection by P for each vertice in the current face.
                zsum += vView.z;
            }
            order.push_back({faceIdx, zsum / 4.f});
            // Standard way of appending ot end of array
        }

        // Camera looks down -Z in OpenGL convention, so "farther" tends to be MORE negative z.
        // Draw far first => sort ascending by z (more negative first).
        std::ranges::sort(order,
                          [](const FaceDraw& a, const FaceDraw& b){ return a.avgZ < b.avgZ; }); // Sort in acsendig order.
                            // Lambda used here the same as when
                            // one wants to declare function inside the int main()

        sf::VertexArray faces(sf::PrimitiveType::Triangles);

        for (auto item : order) {
            const auto& quad = cubeFaces[item.faceIndex];

            sf::Color col = sf::Color(80 + item.faceIndex*20, 140 + item.faceIndex*20, 220);

            std::size_t base = faces.getVertexCount();
            faces.resize(base + 6);

            static constexpr int triPattern[6] = {0,1,2, 0,2,3};
            for (int k = 0; k < 6; ++k) {
                int vidx = quad[triPattern[k]];
                sf::Vector2f p = toScreenH(cubeVerts[vidx], P, MV_cube, windowW, windowH);
                faces[base + k].position = p;
                faces[base + k].color    = col;
            }
        }


        sf::VertexArray grid(sf::PrimitiveType::Lines, 4);
        // std::array<Vec3, 4> vecs_grid = { p0, p1, p2, p3 };
        for ( size_t i = 0 ; i < 4 ; i++ ) {
            grid[i].position = toScreenH(vecs_grid[i], P, MV_plane, windowW, windowH);
        }

        sf::VertexArray triangles(sf::PrimitiveType::Triangles, 6);

        static constexpr int triPattern[6] = { 0, 1, 2,  1, 2, 3 };
        for (int k = 0; k < 6; ++k) {
            auto vid_x = vecs_grid[triPattern[k]];
            sf::Vector2f p = toScreenH(vid_x, P, MV_plane, windowW, windowH);
            triangles[k].position = p;
        }

        UiState uiState{
            .ws = ws,
            .yaw = yaw,
            .pitch = pitch,
            .fovDeg = fovDeg,
            .f = f,
            .sceneScale = sceneScale,
            .aspect = aspect,
            .windowW = windowW,
            .windowH = windowH,
            .v1 = v1,
            .v2 = v2,
            .v3 = v3,
            .u1 = u1,
            .u2 = u2,
            .u3 = u3,
            .a = a,
            .b = b,
            .w = w,
            .MV_base = MV_plane,
            .MV_vectors = MV_plane,
            .P = P,
            .tipVecs = tipVecs
        };
        ShowMatrixLab(uiState);

        window.clear();
        window.draw(triangles);
        window.draw(faces);
        // window.draw(face);
        window.draw(wire);
        window.draw(vecLines);
        window.draw(tips);
        window.draw(&origin, 1, sf::PrimitiveType::Points);
        ImGui::SFML::Render(window); //  [oai_citation:7‡GitHub](https://github.com/SFML/imgui-sfml)
        window.display();
    }

    return 0;
}
