# CPU-Render

A **software 3D renderer written in C++**: every step of the graphics pipeline (vertex processing, clipping, rasterization, shading, depth testing, blending) runs on the **CPU**, with no OpenGL, DirectX or Vulkan involved. SDL3 is only used to open a window and display the final image.

The repository contains both the renderer and a demo scene that shows what it can do: a procedurally generated terrain with water, shadows, particles, a skybox and a small TV model.

## What it does

The project reproduces a GPU-style API (`Graphics`) on top of the CPU, then builds a higher-level renderer (`Renderer`) and a demo engine (`Engine`) on top of it.

### Rendering pipeline (`CPURender/render/base`)

- **GPU-like resource model**: buffers, 2D textures, cube maps, shaders and render targets are created through the `Graphics` class and referenced by `GPU_HANDLE` values.
- **Shaders written in C++**: vertex, pixel and compute shaders are C++ structs declared with the `VERTEX_SHADER`, `PIXEL_SHADER` and `COMPUTE_SHADER` macros. Resources are bound to numbered slots (`BIND_R(...)`), like registers on a real GPU. The API also provides `Draw` and `Dispatch`.
- **Triangle processing**: vertex shader, back-face culling (winding order can be switched at runtime), clipping against the frustum planes plus an optional user clip plane (needed for water reflections).
- **Rasterizer** (`rasterizer.cpp`), parallelized with **OpenMP**:
  - a per-triangle setup pass (screen-space conversion, bounding box, edge/barycentric increments);
  - triangles are binned into screen tiles, which are then rasterized in parallel;
  - perspective-correct interpolation of colors, UVs, world position and TBN matrix;
  - depth buffer test/write, optional alpha blending, pixel discard;
  - screen-space UV derivatives for mip level selection.
- **Texture sampling**: nearest and bilinear filtering, repeat/clamp/mirror wrap modes, mipmaps with automatic LOD selection, cube map sampling.

### Renderer and scene features (`CPURender/render/renderer`)

- **Lighting**: physically based shading (GGX distribution, Smith geometry term, Schlick Fresnel) with albedo, normal, metallic and roughness maps, plus ACES tone mapping. Directional and spot lights, up to 3 at once.
- **Shadow mapping**: one shadow map per light stored in an atlas, with a bias and 3x3 percentage-closer filtering.
- **Water**: animated wave normals, planar reflections (the scene is re-rendered mirrored with a clip plane), refraction of a copy of the scene with depth-based absorption, Fresnel blending, and support for several water bodies.
- **Particles**: billboard particle emitters (smoke, leaves, snow, rain) managed by `ParticuleManager`.
- **Skybox** rendered from a cube map.
- **Frustum culling** with AABB tests, and a **scoped profiler** that times each stage of the frame.
- A **TV model** with a screen quad drawn in a dedicated framebuffer pass.

### Geometry and utilities (`CPURender/common`)

- Procedural meshes: plane, cube, cone, cylinder (`GeoFactory`).
- `.obj` importer.
- Terrain generation from **Perlin noise** (`HeightMap`).
- **Marching Cubes** with a voxel grid and fractal noise density (`common/Marching cubes`). It is implemented but not used in the default demo scene.
- Transforms, frustum/AABB helpers, resource pool, chrono.

## Demo scene (`CPURender/engine/src/engine.cpp`)

The engine builds a small island scene: a 64x64 noise terrain textured from an atlas, two water surfaces, a grass patch with a cube, a cone and a cylinder, smoke, leaves and snow particles, an `.obj` TV model, a directional light and an orange spot light, and a skybox.

The frame is rendered at **640x480** (shown in a 1280x960 window) and capped at **60 FPS**.

## Controls

| Key / input | Action |
|---|---|
| `W` / `Z`, `S` | Move forward / backward |
| `A` / `Q`, `D` | Move left / right |
| `Space` / `Left Ctrl` | Move up / down |
| Mouse | Look around |
| `Esc` | Release / capture the mouse |
| `B` | Toggle the back-face culling winding order |
| `R` | Reset the camera position to the origin |

## Project structure

```
CPURender.sln
Assets/                 textures, skybox faces, particle sprites, .obj models
Libs/
  SDL3/                 window and input
  glm/                  math library
CPURender/
  main.cpp, main.h      SDL window, frame loop, framebuffer presentation
  engine/               demo scene, input handling, per-frame render sequence
  render/
    base/               CPU "GPU": Graphics API, rasterizer, clipping, buffers, shaders
    renderer/           Renderer, camera, lights, materials, textures, shaders
  common/               geometry, terrain, marching cubes, OBJ import, math helpers
  Particule*.cpp/.h     particle system
```

## Building and running

**Requirements**: Windows, Visual Studio 2022 (toolset v143), x64.

1. Open `CPURender.sln` in Visual Studio.
2. Select the `x64` configuration (Debug or Release; Release is much faster for a software renderer).
3. Build and run. SDL3 and GLM are included in `Libs/`.
4. Run with `Assets/` reachable from the working directory, because textures and models are loaded with relative paths such as `Assets/atlas.png`.

The project is compiled as C++17 with `/openmp:llvm`.

## Third-party code

- [SDL3](https://www.libsdl.org/): window creation and input
- [GLM](https://github.com/g-truc/glm): vector and matrix math
- [stb_image](https://github.com/nothings/stb): image loading


## AI Use
- Debugging tools.
- Readme file.