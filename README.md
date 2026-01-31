# Linear Algebra Visual Lab

A real-time 3D visualization tool for exploring linear algebra and computer graphics concepts from the ground up. Implements core rendering algorithms — projection, lighting, shadows — as a software renderer in C++20 with SFML and ImGui.

## Features

- **Software Rendering Pipeline** — World → View → Clip → NDC → Screen coordinate transformations with live matrix inspection
- **Custom LookAt Matrix** — Manual view matrix construction from forward/right/up vectors, toggleable against `glm::lookAt`
- **Orthographic & Perspective Projection** — Switchable projection modes with configurable parameters
- **Phong Flat Shading** — Per-face lighting with ambient, diffuse, and specular components
- **Shadow Projection** — Planar shadow casting using light-source projection matrices
- **Arcball Rotation** — Mouse-driven trackball rotation with momentum/inertia
- **Quaternion Axis Rotation** — Arbitrary-axis rotation via quaternion-to-matrix conversion
- **Orbit Camera** — Spherical coordinate camera with WASD controls
- **ImGui Controls** — Real-time parameter tuning via sliders and toggles

## Tech Stack

| Library | Role |
|---------|------|
| **C++20** | Language standard |
| **SFML 3** | Windowing and 2D rendering |
| **GLM** | Vector/matrix math |
| **ImGui** | Immediate-mode debug UI |

## Building

### Requirements

- CMake 4.1+
- Ninja build system
- C++20 compatible compiler
- SFML 3 (macOS: `brew install sfml`)

### Commands

```bash
cmake --preset debug
cmake --build cmake-build-debug2
./cmake-build-debug2/projection_3d_2d
```

## Controls

| Input | Action |
|-------|--------|
| **Mouse Drag** | Arcball rotation |
| **W / A / S / D** | Camera orbit |
| **ImGui Panel** | Object transform, FOV, projection mode, lighting parameters |
| **Esc** | Exit |

## Project Structure

```
src/
├── app/           Application core — window, input, game loop, rendering
├── math/          Camera, basis transforms, quaternions, lighting, shadows
├── render/        Projection pipeline, mesh data
└── ui/            ImGui debug interface
```

## Implemented Concepts

- Homogeneous coordinates and 4x4 transformation matrices
- View matrix construction (LookAt) and the role of VUP
- Perspective and orthographic projection matrices
- Painter's algorithm for depth-sorted face rendering
- Phong reflection model (ambient, diffuse, specular)
- Planar shadow projection from point light sources
- Arcball rotation via cross product and quaternions
- Change of basis between coordinate systems

## License

MIT