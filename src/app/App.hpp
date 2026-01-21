#pragma once

#include <array>
#include <map>
#include <SFML/Graphics.hpp>

#include "math/Camera.hpp"
#include "math/Types.hpp"
#include "render/Mesh.hpp"

namespace app {

class App {
public:
    App();
    int Run();

private: // Members are initialized in the order they are declared in the class, not the order in the initializer list
    void ProcessEvents();
    void Update(float dt);
    void Render();
    void UpdateControls(float dt);
    float ComputeSceneScale() const;
    std::map<std::tuple<int,int>, sf::Vector2f> barycentricGrid(const glm::vec3& A,
                                       const glm::vec3& B,
                                       const glm::vec3& C,
                                       int N,
                                       const Mat4 &Pr,
                                       const Mat4 &M
                                       ) const;

    unsigned int windowW_{};
    unsigned int windowH_{};
    sf::RenderWindow window_;

    sf::Clock clock_;

    float ws_{};
    float yTrans_{};
    float yaw_{};
    float pitch_{};
    float pitchPlane_{};
    float f_{};
    float fovDeg_{};
    float turnSpeed_{};
    float focalSpeed_{};

    math::OrbitCamera camera_{};

    render::CubeMesh cube_{};
    std::array<Vec3, 4> grid_{};
    // std::vector<glm::vec3>;

    std::array<Vec3, 3> vBasis_{};
    std::array<Vec3, 3> uBasis_{};
    Vec3 a_{};
    Vec3 b_{};
    Vec3 w_{};
    Vec3 originWorld_{};

    bool printed_{};
};

} // namespace app
