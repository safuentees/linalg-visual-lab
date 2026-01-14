#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

// ---------- small Vec3 helpers (no reliance on operator overloads) ----------
static sf::Glsl::Vec3 add(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
static sf::Glsl::Vec3 sub(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
static sf::Glsl::Vec3 mul(const sf::Glsl::Vec3& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}
static float dot(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static sf::Glsl::Vec3 cross(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}
static float maxAbsComponent(const sf::Glsl::Vec3& v) {
    return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
}

// Compute coordinates x in basis (e1,e2,e3) such that:
// w = x1*e1 + x2*e2 + x3*e3
// Using: det = e1 · (e2 × e3)
// x1 = w · (e2 × e3) / det, etc.
static sf::Glsl::Vec3 coordsInBasis(const sf::Glsl::Vec3& e1,
                                    const sf::Glsl::Vec3& e2,
                                    const sf::Glsl::Vec3& e3,
                                    const sf::Glsl::Vec3& w) // by what scalars/vector (c1, c2, c3) do I need to multiply the basis the vectors in order to get to this position (w1, w2, w3).
{
    const sf::Glsl::Vec3 e2xe3 = cross(e2, e3);
    const sf::Glsl::Vec3 e3xe1 = cross(e3, e1);
    const sf::Glsl::Vec3 e1xe2 = cross(e1, e2);

    const float detA = dot(e1, e2xe3);
    if (std::abs(detA) < 1e-6f) {
        return {0.f, 0.f, 0.f}; // degenerate basis
    }

    return {
        dot(w, e2xe3) / detA,
        dot(w, e3xe1) / detA,
        dot(w, e1xe2) / detA
    };
}

static sf::Glsl::Vec3 fromCoords(const sf::Glsl::Vec3& e1,
                                 const sf::Glsl::Vec3& e2,
                                 const sf::Glsl::Vec3& e3,
                                 const sf::Glsl::Vec3& c)
{
    return add(add(mul(e1, c.x), mul(e2, c.y)), mul(e3, c.z));
}

void controls (float& f, float focalSpeed, float dt, float& ws, float turnSpeed, float& yaw, float& pitch) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) f -= focalSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) f += focalSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) ws -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) ws += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  yaw   -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) yaw   += turnSpeed * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))  pitch -= turnSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))    pitch += turnSpeed * dt;
}

int main() {
    // ---------- camera-ish params (your essence) ----------
    float ws    = 2.f;   // translate scene forward so z > 0 (avoid divide-by-0)
    float yaw   = 7.f;
    float pitch = 7.f;
    float f     = 1.f;   // focal scalar

    sf::Clock clock;
    const float turnSpeed  = 1.f;
    const float focalSpeed = 1.f;

    constexpr unsigned int kWindowW = 800;
    constexpr unsigned int kWindowH = 800;
    sf::RenderWindow window(sf::VideoMode({kWindowW, kWindowH}), "SFML window");

    // ---------- projection / transforms ----------
    auto project = [&](sf::Glsl::Vec3 p) -> sf::Vector2f {
        return { f * (p.x / p.z), f * (p.y / p.z) };
    };

    auto rotate_y = [&](sf::Glsl::Vec3 p, float a) -> sf::Glsl::Vec3 {
        float c = std::cos(a), s = std::sin(a);
        return { c*p.x + s*p.z, p.y, -s*p.x + c*p.z };
    };

    auto rotate_x = [&](sf::Glsl::Vec3 p, float a) -> sf::Glsl::Vec3 {
        float c = std::cos(a), s = std::sin(a);
        return { p.x, p.y*c + p.z*s, -p.y*s + p.z*c };
    };

    auto translate_z = [&](sf::Glsl::Vec3 p, float dz) -> sf::Glsl::Vec3 {
        p.z += dz;
        return p;
    };

    auto screen = [&](sf::Vector2f p) -> sf::Vector2f {
        return {
            (p.x + 1.f) * 0.5f * kWindowW,
            (1.f - (p.y + 1.f) * 0.5f) * kWindowH
        };
    };

    // One place to apply: scale -> yaw -> pitch -> translate -> project -> screen
    auto toScreen = [&](sf::Glsl::Vec3 world, float sceneScale) -> sf::Vector2f {
        world = mul(world, sceneScale);
        world = rotate_y(world, yaw);
        world = rotate_x(world, pitch);
        world = translate_z(world, ws);

        // Optional safety: if behind camera, shove offscreen
        if (world.z <= 0.001f) return {-99999.f, -99999.f};

        return screen(project(world));
    };

    // ---------- cube wireframe (same idea as you had) ----------
    const float zNear = -0.5f;
    const float zFar  =  0.5f;

    const std::array<sf::Glsl::Vec3, 8> cubeVerts = {{
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
    // Choose a v-basis in world space (start with standard basis)
    // You can replace these with whatever the book gives you later.
    sf::Glsl::Vec3 v1{1.f, 0.f, 0.f};
    sf::Glsl::Vec3 v2{0.f, 1.f, 0.f};
    sf::Glsl::Vec3 v3{0.f, 0.f, 1.f};

    // Coordinates a in v-basis (book example a = [1,2,3]^T)
    sf::Glsl::Vec3 a{1.f, 2.f, 3.f};
    sf::Glsl::Vec3 w = fromCoords(v1, v2, v3, a);
    sf::Glsl::Vec4 d = {1,2,3,4};
    // New basis from the book:
    // u1 = v1
    // u2 = v1 + v2
    // u3 = v1 + v2 + v3
    sf::Glsl::Vec3 u1 = v1;
    sf::Glsl::Vec3 u2 = add(v1, v2);
    sf::Glsl::Vec3 u3 = add(add(v1, v2), v3);

    // Find b s.t. w = b1*u1 + b2*u2 + b3*u3
    sf::Glsl::Vec3 b = coordsInBasis(u1, u2, u3, w);
    // (For the book example you should get approximately {-1, -1, 3}).

    // ---------- global scene scale (preserve ratios!) ----------
    // We want v/u/w to fit inside the cube of half-extent ~0.45.
    auto computeSceneScale = [&]() -> float {
        std::array<sf::Glsl::Vec3, 7> vecs = { v1, v2, v3, u1, u2, u3, w };

        float m = 0.f;
        for (const auto& t : vecs) m = std::max(m, maxAbsComponent(t));

        const float halfBox = 0.5f;
        return (m > 0.f) ? (halfBox / m) : 1.f;
    };

    // Helper to add a vector segment origin->vec into a VertexArray(Lines)
    auto addVectorLine = [&](sf::VertexArray& va,
                             const sf::Glsl::Vec3& vec,
                             float sceneScale,
                             sf::Color color)
    {
        const sf::Vector2f O = toScreen({0.f,0.f,0.f}, sceneScale);
        const sf::Vector2f P = toScreen(vec, sceneScale);

        const std::size_t base = va.getVertexCount();
        va.resize(base + 2); // Extend existing array of vectors (maybe i j k): i j k _ _ , (i j k (O,P)).
        va[base + 0].position = O;     //                                                       ^ ^ (+2)
        va[base + 1].position = P;     //                                                       O P
        va[base + 0].color = color;
        va[base + 1].color = color;
    };

    // Optional: show b on console sometimes
    bool printed = false;

    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
        }

        // controls (same as you)
        controls(f, focalSpeed, dt, ws, turnSpeed, yaw, pitch);

        // recompute scale each frame (cheap, keeps things consistent)
        const float sceneScale = computeSceneScale();

        // rebuild cube wireframe
        sf::VertexArray wire(sf::PrimitiveType::Lines);
        wire.resize(cubeEdges.size() * 2);

        for (std::size_t e = 0; e < cubeEdges.size(); ++e) {
            auto [aIdx, bIdx] = cubeEdges[e];
            sf::Vector2f A = toScreen(cubeVerts[aIdx], 1.f); // cube stays fixed scale
            sf::Vector2f B = toScreen(cubeVerts[bIdx], 1.f);

            wire[2*e + 0].position = A;
            wire[2*e + 1].position = B;
            wire[2*e + 0].color = sf::Color::Green;
            wire[2*e + 1].color = sf::Color::Green;
        }

        // build vector drawings
        sf::VertexArray vecLines(sf::PrimitiveType::Lines);

        // v-basis (RGB)
        addVectorLine(vecLines, v1, sceneScale, sf::Color::Red);
        addVectorLine(vecLines, v2, sceneScale, sf::Color::Green);
        addVectorLine(vecLines, v3, sceneScale, sf::Color::Blue);

        // u-basis (lighter colors)
        addVectorLine(vecLines, u1, sceneScale, sf::Color(255, 140, 140));
        addVectorLine(vecLines, u2, sceneScale, sf::Color(140, 255, 140));
        addVectorLine(vecLines, u3, sceneScale, sf::Color(140, 140, 255));

        // w (white)
        addVectorLine(vecLines, w, sceneScale, sf::Color::White);

        // points: show tips of vectors if you want
        sf::VertexArray tips(sf::PrimitiveType::Points);
        tips.resize(7);
        std::array<sf::Glsl::Vec3, 7> tipVecs = { v1,v2,v3,u1,u2,u3,w };
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

        // draw cycle: clear -> draw -> display  [oai_citation:2‡SFML](https://www.sfml-dev.org/tutorials/3.0/graphics/draw/)
        window.clear();
        window.draw(wire);
        window.draw(vecLines);
        window.draw(tips);
        window.display();
    }

    return 0;
}