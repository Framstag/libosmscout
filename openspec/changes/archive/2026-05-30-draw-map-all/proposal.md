## Why

Currently each map rendering backend (AGG, Cairo, OpenGL, Qt, SVG, DirectX, GDI, OSX) has its own DrawMap application with near-identical argument handling. Comparing rendering output across backends requires running 8 separate commands. A single unified command that renders with all available backends and stores results in a directory simplifies testing, regression checks, and visual comparison.

## What Changes

- New `DrawMapAll` demo application under `Demos/` that renders the same map view using every available rendering backend in a single invocation
- Output goes to a specified directory, one file per backend (`DrawMapAgg.png`, `DrawMapCairo.png`, `DrawMapOpenGL.png`, etc.)
- Raster backends produce PNG via libpng; SVG backend produces `.svg`
- Single C++ source file with `#ifdef`-based conditional compilation per backend
- Build system integration (CMake + Meson) that detects available backends and only includes the ones present
- No changes to existing DrawMap applications

## Capabilities

### New Capabilities
- `multi-backend-render`: Application that accepts same parameters as existing DrawMap apps but renders using all available backends and writes results to a directory

### Modified Capabilities

None.

## Impact

- **New file**: `Demos/src/DrawMapAll.cpp` — main application
- **Modified**: `Demos/CMakeLists.txt` — add DrawMapAll target with conditional backend linkage
- **Modified**: `Demos/meson.build` — add DrawMapAll target with conditional backend linkage
- **Modified**: `Demos/include/DrawMap.h` — possible minor additions (OpenGL shader path arg, output directory arg)
- **libpng**: Required dependency for RGB→PNG conversion (AGG, OpenGL backends), already a project dependency
- **GLFW**: Required for OpenGL offscreen rendering (already required for DrawMapOpenGL)
- **Windows-specific**: Offscreen rendering paths for DirectX (WIC bitmap) and GDI (DIBSection)
- **macOS-specific**: DrawMapOSX code uses ObjC++ (`.mm`)
