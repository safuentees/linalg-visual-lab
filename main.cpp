#include <iostream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics.hpp>

#include <array>
#include <cmath>

int main()
{
    // Angle so k-axis isn’t pointing straight “into” the camera
    float ps   = 0.f;
    float ws   = 2.f;   // offset Z by +1 so you dont divide by 0

    float yaw   = 0.f;   // radians (~20°)
    float pitch = 0.f;  // radians (~-14°)
    
    bool show_cubes = false;

    sf::Clock clock;
    float turnSpeed  = 1.f; // radians per second
    float focalSpeed = 1.f; // radians per second

    sf::VertexArray basis(sf::PrimitiveType::Lines, 6);

    float d { 100.f };

    // ---------- Window ----------
    constexpr unsigned int kWindowW = 800;
    constexpr unsigned int kWindowH = 800;
    const sf::Vector2u windowSize{ kWindowW, kWindowH };
    float zNear = -0.5f;
    float zFar  =  0.5f; // centered at origin

    float f = 1.f;

    auto project = [&](sf::Glsl::Vec3 p) {
        return sf::Vector2f { f*(p.x / p.z), f*(p.y / p.z) };
    };

    // x = xcos(theta) - zsin(theta), z = xsin(theta) + zcos(theta)
    auto rotate_y = [&](sf::Glsl::Vec3 p, float a) {
        float c = std::cos(a), s = std::sin(a);
        return sf::Glsl::Vec3(
            c*p.x + s*p.z,
            p.y,
            -s*p.x + c*p.z
        );
    };

    auto screen = [&](sf::Vector2f p) {
        return sf::Vector2f(
            (p.x + 1)/2 * kWindowW,
            (1 - (p.y + 1)/2) * kWindowH
        );
    };

    // Translation formula (same idea as translate_z in the canvas tutorial)
    auto translate_z = [&](sf::Glsl::Vec3 p, float dz) {
        p.z += dz;
        return p;
    };

    // --- Refactor: 8 rectangles in an array ---
    constexpr float rectSize = 16.f;
    std::array<sf::RectangleShape, 8> recs;

    for (auto& r : recs)
    {
        r.setSize({rectSize, rectSize});
        r.setOrigin({rectSize / 2.f, rectSize / 2.f});
    }

    // Same 8 points you were drawing before, in the same order:
    // rec, rec1, rec2, rec3, recd, rec1d, rec2d, rec3d
    // 0..3 = near face (z = zNear), 4..7 = far face (z = zFar)
    std::array<sf::Glsl::Vec3, 8> verts = {{
        {-0.5f,  0.5f,  zNear}, // 0
        { 0.5f,  0.5f,  zNear}, // 1
        { 0.5f, -0.5f,  zNear}, // 2
        {-0.5f, -0.5f,  zNear}, // 3
        {-0.5f,  0.5f,  zFar }, // 4
        { 0.5f,  0.5f,  zFar }, // 5
        { 0.5f, -0.5f,  zFar }, // 6
        {-0.5f, -0.5f,  zFar }  // 7
    }};

    std::array<sf::Glsl::Vec3, 8> verts_basis_points = {{
        {0,  -0.5,  0}, // 0
        { 0.5,  0,  0}, // 1
        { -0.5, 0, 0}, // 2
        {-0.5, 0, 0}
    }};

    sf::Glsl::Vec3 P0(0, -0.5, 0);
    sf::Glsl::Vec3 P1(0.5, 0, 0);
    sf::Glsl::Vec3 P2(-0.5, 0, 0);
    sf::Glsl::Vec3 P3(-0.5, 0, 0);

    sf::VertexArray points(sf::PrimitiveType::Points);

    constexpr std::array<std::pair<int,int>, 12> edges = {{
        {0,1}, {1,2}, {2,3}, {3,0},  // near face
        {4,5}, {5,6}, {6,7}, {7,4},  // far face
        {0,4}, {1,5}, {2,6}, {3,7}   // connections between faces
    }};

    sf::VertexArray lines_sq(sf::PrimitiveType::Lines, 13);

    sf::RenderWindow window(sf::VideoMode(windowSize), "SFML window");


    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        // Hold A/D to rotate
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            f -= focalSpeed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            f += focalSpeed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            ws -= turnSpeed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            ws += turnSpeed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            yaw -= turnSpeed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            yaw += turnSpeed * dt;
        }

        points.resize(verts_basis_points.size());

        sf::Vector2f O  = screen(project(translate_z(rotate_y(P0,yaw), ws)));
        sf::Vector2f O1 = screen(project(translate_z(rotate_y(P1,yaw), ws)));
        sf::Vector2f O2 = screen(project(translate_z(rotate_y(P2,yaw), ws)));
        sf::Vector2f O3 = screen(project(translate_z(rotate_y(P3,yaw), ws)));

        points[0].position = O, points[1].position = O1, points[2].position = O2, points[3].position = O3;

        for (auto verts_basis_point : verts_basis_points)
        {
            auto rot = rotate_y(verts_basis_point, yaw);
            auto trans = translate_z(rot, ws);
            points[0].position = screen(project(trans));
        }

        // --- Refactor: set positions via loop + translation formula ---
        if (show_cubes) {
            for (std::size_t idx = 0; idx < recs.size(); ++idx)
            {
                auto rot = rotate_y(verts[idx], yaw);
                auto trans = translate_z(rot, ws);
                recs[idx].setPosition(screen(project(trans)));
            }
        }

        sf::VertexArray wire(sf::PrimitiveType::Lines);
        wire.resize(edges.size() * 2);
        
        for (size_t e = 0; e < edges.size(); ++e) {
            auto [a,b] = edges[e];

            auto rot_a = rotate_y(verts[a], yaw);
            auto trans_a = translate_z(rot_a, ws);

            auto rot_b = rotate_y(verts[b], yaw);
            auto trans_b = translate_z(rot_b, ws);

            sf::Vector2f A = screen(project( trans_a ));
            sf::Vector2f B = screen(project( trans_b ));

            wire[2*e + 0].position = A;
            wire[2*e + 1].position = B;
            wire[2*e + 0].color = sf::Color::Green;
            wire[2*e + 1].color = sf::Color::Green;
        }

        window.clear();
        window.draw(wire);

        window.draw(points);

        for (auto& r : recs)
            window.draw(r);

        window.draw(lines_sq);
        window.display();
    }
    auto pos = recs[5].getPosition();
    printf("%f, %f\n",pos.x, pos.y);
}