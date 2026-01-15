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

void controls(float& f, float focalSpeed, float dt, float& ws, float turnSpeed, float& yaw, float& pitch) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) f -= focalSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) f += focalSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) ws -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) ws += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  yaw   -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) yaw   += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    pitch -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  pitch += turnSpeed * dt;
}

int main() {
    float ws    = 2.f;
    float yaw   = 7.f;
    float pitch = 7.f;
    float f     = 1.f;

    sf::Clock clock;
    const float turnSpeed  = 1.f;
    const float focalSpeed = 1.f;

    constexpr unsigned int kWindowW = 800;
    constexpr unsigned int kWindowH = 800;
    sf::RenderWindow window(sf::VideoMode({kWindowW, kWindowH}), "SFML window");

    // ---------- projection / transforms (still your manual ones for now) ----------
    auto project = [&](Vec3 p) -> sf::Vector2f {
        return { f * (p.x / p.z), f * (p.y / p.z) };
    };

    auto rotate_y = [&](Vec3 p, float a) -> Vec3 {
        float c = std::cos(a), s = std::sin(a);
        return { c*p.x + s*p.z, p.y, -s*p.x + c*p.z };
    };

    auto rotate_x = [&](Vec3 p, float a) -> Vec3 {
        float c = std::cos(a), s = std::sin(a);
        return { p.x, p.y*c + p.z*s, -p.y*s + p.z*c };
    };

    auto translate_z = [&](Vec3 p, float dz) -> Vec3 {
        p.z += dz;
        return p;
    };

    auto screen = [&](sf::Vector2f p) -> sf::Vector2f {
        return {
            (p.x + 1.f) * 0.5f * kWindowW,
            (1.f - (p.y + 1.f) * 0.5f) * kWindowH
        };
    };

    auto toScreen = [&](Vec3 world, float sceneScale) -> sf::Vector2f {
        world *= sceneScale;
        world = rotate_y(world, yaw);
        world = rotate_x(world, pitch);
        world = translate_z(world, ws);

        if (world.z <= 0.001f) return {-99999.f, -99999.f};
        return screen(project(world));
    };

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

    auto addVectorLine = [&](sf::VertexArray& va,
                             const Vec3& vec,
                             float sceneScale,
                             sf::Color color)
    {
        Vec3 originWorld{0.f, 5.f, 0.f};
        Vec3 headWorld = originWorld + vec;

        const sf::Vector2f O = toScreen(originWorld, sceneScale);
        const sf::Vector2f P = toScreen(headWorld,   sceneScale);

        const std::size_t base = va.getVertexCount();
        va.resize(base + 2);
        va[base + 0].position = O;
        va[base + 1].position = P;
        va[base + 0].color = color;
        va[base + 1].color = color;
    };

    bool printed = false;

    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
        }

        controls(f, focalSpeed, dt, ws, turnSpeed, yaw, pitch);

        const float sceneScale = computeSceneScale();

        sf::VertexArray wire(sf::PrimitiveType::Lines);
        wire.resize(cubeEdges.size() * 2);

        for (std::size_t e = 0; e < cubeEdges.size(); ++e) {
            auto [aIdx, bIdx] = cubeEdges[e];
            sf::Vector2f A = toScreen(cubeVerts[aIdx], 1.f);
            sf::Vector2f B = toScreen(cubeVerts[bIdx], 1.f);

            wire[2*e + 0].position = A;
            wire[2*e + 1].position = B;
            wire[2*e + 0].color = sf::Color::Green;
            wire[2*e + 1].color = sf::Color::Green;
        }

        sf::VertexArray vecLines(sf::PrimitiveType::Lines);

        addVectorLine(vecLines, v1, sceneScale, sf::Color::Red);
        addVectorLine(vecLines, v2, sceneScale, sf::Color::Green);
        addVectorLine(vecLines, v3, sceneScale, sf::Color::Blue);

        addVectorLine(vecLines, u1, sceneScale, sf::Color(255, 140, 140));
        addVectorLine(vecLines, u2, sceneScale, sf::Color(140, 255, 140));
        addVectorLine(vecLines, u3, sceneScale, sf::Color(140, 140, 255));

        addVectorLine(vecLines, w,  sceneScale, sf::Color::White);

        sf::VertexArray tips(sf::PrimitiveType::Points);
        tips.resize(7);
        std::array<Vec3, 7> tipVecs = { v1,v2,v3,u1,u2,u3,w };
        for (std::size_t i = 0; i < tipVecs.size(); ++i) {
            tips[i].position = toScreen(tipVecs[i], sceneScale);
            tips[i].color = sf::Color::White;
        }

        if (!printed) {
            printed = true;
            std::cout << "Book example: a=[1,2,3] in v-basis\n";
            std::cout << "Computed w = (" << w.x << "," << w.y << "," << w.z << ")\n";
            std::cout << "Computed b in u-basis ~ (" << b.x << "," << b.y << "," << b.z << ")\n";
        }

        window.clear();
        window.draw(wire);
        window.draw(vecLines);
        window.draw(tips);
        window.display();
    }

    return 0;
}