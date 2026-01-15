#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

// Put near the top with your typedefs
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

void controls(float& f, float focalSpeed, float dt, float& ws, float turnSpeed, float& yaw, float& pitch, float& fovdeg) {
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
}

int main() {
    float ws    = 2.f;
    float yaw   = 7.f;
    float pitch = 7.f;
    float f     = 1.f;
    float fovDeg = 45.f; // make this a variable

    sf::Clock clock;
    const float turnSpeed  = 1.f;
    const float focalSpeed = 30.f;

    constexpr unsigned int kWindowW = 800;
    constexpr unsigned int kWindowH = 800;
    sf::RenderWindow window(sf::VideoMode({kWindowW, kWindowH}), "SFML window"); //, sf::State::Fullscreen

    // ---------- cube wireframe ----------
    const float zNear = -0.5f;
    const float zFar  =  0.5f;

    const std::array<Vec3, 8> cubeVerts = {{
        {-0.5f,  0.5f,  zNear}, { 0.5f,  0.5f,  zNear},
        { 0.5f, -0.5f,  zNear}, {-0.5f, -0.5f,  zNear},
        {-0.5f,  0.5f,  zFar }, { 0.5f,  0.5f,  zFar },
        { 0.5f, -0.5f,  zFar }, {-0.5f, -0.5f,  zFar }
    }};

    constexpr std::array<std::pair<int,int>, 12> cubeEdges = {{
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    }};

    // ---------- TEXTBOOK LAYER ----------
    Vec3 v1{1.f, 0.f, 0.f};
    Vec3 v2{0.f, 1.f, 0.f};
    Vec3 v3{0.f, 0.f, 1.f};

    Vec3 a{1.f, 2.f, 3.f};
    Vec3 w = fromCoords(v1, v2, v3, a);

    Vec3 u1 = v1;
    Vec3 u2 = v1 + v2;
    Vec3 u3 = v1 + v2 + v3;

    Vec3 b = coordsInBasis(u1, u2, u3, w);

    auto computeSceneScale = [&]() -> float {
        std::array<Vec3, 7> vecs = { v1, v2, v3, u1, u2, u3, w };

        float m = 0.f;
        for (const auto& t : vecs) m = std::max(m, maxAbsComponent(t));

        const float halfBox = 0.5f;
        return (m > 0.f) ? (halfBox / m) : 1.f;
    };

    bool printed = false;

    while (window.isOpen()) {
        int count = 0;
        const float dt = clock.restart().asSeconds();

        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
        }

        controls(f, focalSpeed, dt, ws, turnSpeed, yaw, pitch, fovDeg);

        const float sceneScale = computeSceneScale();

        // ---- Build MV (matches your old order: scale -> yaw -> pitch -> translate) ----
        // Old code did: world*=sceneScale; world=Ry; world=Rx; world.z += ws;
        // Equivalent matrix: MV = T * Rx * Ry * S
        Mat4 MV_base(1.f);
        MV_base = glm::translate(MV_base, Vec3(0,0,-ws));
        MV_base = glm::rotate(MV_base, pitch, Vec3(1,0,0));
        MV_base = glm::rotate(MV_base, yaw,   Vec3(0,1,0));

        Mat4 MV_vectors = glm::scale(MV_base, Vec3(sceneScale));
        Mat4 MV_cube    = MV_base;  // no scale
        // ---- Projection ----
        float aspect = float(kWindowW) / float(kWindowH);

        Mat4 P = glm::perspective(glm::radians(fovDeg), aspect, 0.01f, 100.f);

        auto addVectorLine = [&](sf::VertexArray& va, const Vec3& vec, sf::Color color)
        {
            Vec3 originWorld{1.f, 2.f, 3.f}; // Translation Vector in Matrix
            Vec3 headWorld   = originWorld + vec;

            sf::Vector2f O = toScreenH(originWorld, P, MV_vectors, kWindowW, kWindowH);
            sf::Vector2f H = toScreenH(headWorld,   P, MV_vectors, kWindowW, kWindowH);

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
            if (toScreenH(cubeVerts[aIdx], P, MV_cube, kWindowW, kWindowH, A) &&
                toScreenH(cubeVerts[bIdx], P, MV_cube, kWindowW, kWindowH, B))
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
            tips[i].position = toScreenH(tipVecs[i], P, MV_vectors, kWindowW, kWindowH);
            tips[i].color = sf::Color::White;
        }

        if (!printed) {
            printed = true;
            std::cout << "Book example: a=[1,2,3] in v-basis\n";
            std::cout << "Computed w = (" << w.x << "," << w.y << "," << w.z << ")\n";
            std::cout << "Computed b in u-basis ~ (" << b.x << "," << b.y << "," << b.z << ")\n";
        }



        sf::VertexArray face(sf::PrimitiveType::Triangles);
        face.resize(6);
        //                               0 1 2
        // •	Triangle 1: (q0, q1, q2) 0 1 2
        // •	Triangle 2: (q0, q2, q3) 0 1+1 2+1
        for (std::size_t j = 0; j < 2; ++j) {
            for (std::size_t e = 0; e < 3; ++e) {
                size_t idx = e == 0 ? e : e + j;
                sf::Vector2f p = toScreenH(cubeVerts[idx], P, MV_cube, kWindowW, kWindowH);
                face[count].position = p;
                count++;
            }
        }

        // sf::Vector2f p0 = toScreenH(cubeVerts[0], P, MV_cube, kWindowW, kWindowH);
        // sf::Vector2f p1 = toScreenH(cubeVerts[1], P, MV_cube, kWindowW, kWindowH);
        // sf::Vector2f p2 = toScreenH(cubeVerts[2], P, MV_cube, kWindowW, kWindowH);
        // sf::Vector2f p3 = toScreenH(cubeVerts[3], P, MV_cube, kWindowW, kWindowH);
        //
        // // triangle (0,1,2)
        // face[0].position = p0; face[1].position = p1; face[2].position = p2;
        // // triangle (0,2,3)
        // face[3].position = p0; face[4].position = p2; face[5].position = p3;

        // flat color (same on all vertices)
        for (int i = 0; i < 6; ++i) face[i].color = sf::Color(80, 120, 255);

        window.clear();
        window.draw(face);
        window.draw(wire);
        window.draw(vecLines);
        window.draw(tips);
        window.display();
    }

    return 0;
}