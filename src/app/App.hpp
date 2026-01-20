#pragma once

#include <array>

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
    static std::vector<glm::vec3> barycentricGrid(const glm::vec3& A,
                                                  const glm::vec3& B,
                                                  const glm::vec3& C,
                                                  int N);

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

    Vec3 v1_{};
    Vec3 v2_{};
    Vec3 v3_{};
    Vec3 u1_{};
    Vec3 u2_{};
    Vec3 u3_{};
    Vec3 a_{};
    Vec3 b_{};
    Vec3 w_{};
    Vec3 originWorld_{};

    bool printed_{};
};

} // namespace app
