# Linear Algebra Visual Lab

An interactive 3D-to-2D projection visualization tool for exploring linear algebra concepts. Built with C++20, SFML, and ImGui.


## Features

- **Real-time 3D Projection** - Visualize the complete graphics pipeline: World → View → Clip → NDC → Screen coordinates
- **Arcball Rotation** - Intuitive mouse-based rotation with inertia physics
- **Orbit Camera** - Spherical coordinate camera system with yaw/pitch controls
- **Matrix Visualization** - ImGui debug windows showing live matrix transformations
- **Basis Vector Display** - Visualize coordinate system transformations
- **Painter's Algorithm** - Depth-sorted face rendering for correct occlusion

## Tech Stack

- **C++20** - Modern C++ features
- **SFML 3** - Window management and 2D rendering
- **GLM** - Mathematics library (vectors, matrices, transformations)
- **ImGui** - Immediate mode debug UI

## Building

### Requirements

- CMake 4.1+
- Ninja build system
- C++20 compatible compiler
- SFML 3 (via Homebrew on macOS: `brew install sfml`)

### Build Commands

```bash
# Configure
cmake --preset debug

# Build
cmake --build cmake-build-debug2

# Run
./cmake-build-debug2/projection_3d_2d
```

## Controls

| Input | Action |
|-------|--------|
| **Mouse Drag** | Arcball rotation |
| **Mouse Release** | Inertia spin |
| W / S | Zoom in/out |
| A / D | Adjust focal length |
| Q / E | Adjust FOV |
| Arrow Keys | Yaw/Pitch rotation |
| P / O | Camera orbit yaw |
| L / K | Camera orbit pitch |
| Z / X | Y-axis translation |
| 9 / 0 | Axis-angle rotation |
| Esc | Exit |

## Project Structure

```
src/
├── app/           # Application core (window, input, game loop)
├── math/          # Camera, basis transformations, type aliases
├── render/        # Projection pipeline, mesh data
└── ui/            # ImGui debug interface
```

## Learning Goals

This project was built to understand:

- Homogeneous coordinate systems and 4x4 transformation matrices
- The complete 3D rendering pipeline
- Arcball rotation using cross/dot products
- Perspective projection and NDC conversion
- Change of basis between coordinate systems

## License

MIT
