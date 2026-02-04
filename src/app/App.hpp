#pragma once

#include <SFML/Graphics.hpp>

#include "app/SceneParams.hpp"
#include "math/Camera.hpp"
#include "render/Mesh.hpp"

namespace app {

class App {
public:
    App();
    int Run();

private:
    void ProcessEvents(float dt);
    void Update(float dt);
    void Render();
    void UpdateControls(float dt);
    float ComputeSceneScale() const;

    // Window (windowW_/windowH_ must be declared before window_ for initialization order)
    unsigned int windowW_{};
    unsigned int windowH_{};
    sf::RenderWindow window_;
    sf::Clock clock_;

    // Grouped state
    MaterialParams material_;
    TransformParams transform_;
    ViewParams view_;
    ControlSettings controls_;
    SceneGeometry scene_;

    // Objects
    math::OrbitCamera camera_;
    render::CubeMesh cube_;

    // Debug
    bool printed_ = false;
};

} // namespace app
