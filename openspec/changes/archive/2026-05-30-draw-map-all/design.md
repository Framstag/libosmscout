## Context

`DrawMapAll` is a new Demo application that renders the same map view through every available rendering backend in a single invocation. It reuses the existing shared infrastructure (`DrawMap.h` — argument parser, `DatabaseEntry`, `DrawMapDemo` base class) and mirrors the existing per-backend DrawMap implementations.

Each backend has a distinct rendering API:
- **AGG**: `MapPainterAgg::DrawMap(AggPixelFormat*)` — renders into raw RGB buffer
- **Cairo**: `MapPainterCairo::DrawMap(cairo_t*)` — renders to Cairo surface
- **OpenGL**: `MapPainterOpenGL` — needs GLFW offscreen context, `glReadPixels` to extract RGB
- **Qt**: `MapPainterQt::DrawMap(QPainter*)` — renders to QPixmap
- **SVG**: `MapPainterSVG::DrawMap(ostream&)` — writes SVG text
- **OSX**: `MapPainterIOS::DrawMap(CGContextRef)` — renders to CoreGraphics context
- **DirectX**: `MapPainterDirectX::DrawMap(ID2D1RenderTarget*)` — needs Direct2D render target
- **GDI**: `MapPainterGDI::DrawMap(HDC)` — needs GDI device context

## Goals / Non-Goals

**Goals:**
- Single command renders map with all compiled-in backends
- Same arguments as existing DrawMap apps (stylesheet, center, zoom, etc.)
- Output directory contains one file per backend (`DrawMapBackend.png` or `.svg`)
- PNG format for all raster backends via libpng
- SVG format for SVG backend
- Backend detection at build time via CMake/Meson compile definitions
- AGG raw RGB → libpng; OpenGL raw RGB (Y-flipped) → libpng
- Offscreen rendering for DirectX (WIC bitmap) and GDI (DIBSection)

**Non-Goals:**
- No changes to existing per-backend DrawMap apps
- No new rendering capabilities or style features
- No real-time window display (all offscreen)
- No SVG rasterization to PNG

## Decisions

### Single file with conditional compilation

One `DrawMapAll.cpp` with `#ifdef HAVE_OSMSCOUT_MAP_<BACKEND>` blocks per backend. Each block is self-contained: create offscreen resources, render, save PNG, cleanup.

**Rationale**: All backends share the same `DrawMapDemo` base data (projection, parameters, map data). A shared setup phase then dispatches to backend-specific rendering blocks. No benefit to splitting across files — the platform-specific `#ifdef` guards (`#ifdef _WIN32` for DirectX/GDI, `__APPLE__` for OSX) are cleaner inline.

### Libpng for raw RGB → PNG conversion

AGG produces `agg::pixfmt_rgb24` (RGB8, top-down). OpenGL produces `glReadPixels` (RGB8, bottom-up). Both convert via `png_write_row()`.

**Rationale**: Libpng already required by Cairo and OpenGL backends. No new dependency. A helper `WriteRGBToPNG()` function handles row order (top-down vs bottom-up), 8-bit depth, RGBA→RGB strip.

### AGG RGB buffer: no pixel format conversion

AGG's `pixfmt_rgb24` is already packed RGB8. Buffer extracted via `rendering_buffer::buf()` and passed directly to libpng. No intermediate format needed.

### OpenGL offscreen context via GLFW

Same pattern as existing `DrawMapOpenGL.cpp`: create hidden GLFW window, make context current, render, `glReadPixels`, convert to PNG, destroy.

**Rationale**: GLFW is already a dependency of DrawMapOpenGL. The hidden window approach requires no platform-specific GL context creation code.

### Qt: minimal QApplication

`QApplication` must be constructed before rendering. Create with minimal args, no windows shown. `QPixmap` + `QPainter` → `QPixmap::save("PNG")`. `QApplication` destroyed after all backends render.

### DirectX offscreen: WIC bitmap render target

Create `IWICBitmap` → `ID2D1Factory::CreateWicBitmapRenderTarget()` → `MapPainterDirectX::DrawMap()` → `IWICBitmapEncoder` to write PNG file.

This avoids creating a visible window (contrast with existing `DrawMapDirectX` which uses `CreateWindow` + message loop).

### GDI offscreen: DIBSection

`CreateCompatibleDC()` + `CreateDIBSection()` → `SelectObject()` → render via `MapPainterGDI::DrawMap(HDC)` → save PNG via `Gdiplus::Bitmap::Save()`.

Existing `DrawMapGDI` uses `MapPainterGDIWindow` (interactive window). The DIBSection approach provides an offscreen HDC.

### SVG: native format, not PNG

SVG backend writes vector text to a stream. No attempt to rasterize. Output file gets `.svg` extension instead of `.png`.

## Execution Flow

```
Parse args (via shared DrawMapArgParser)
  │
OpenDatabase() + LoadData()
  │
Create output directory (mkdir -p)
  │
┌───── For each available backend ─────┐
│                                       │
│  Init backend resources               │
│  Render map to backend target         │
│  If raster: extract RGB buffer        │
│  Convert RGB → PNG via libpng         │
│  Write <outputDir>/DrawMap<B>.png     │
│  Cleanup backend resources            │
│                                       │
└───────────────────────────────────────┘
```

## Risks / Trade-offs

- **[DirectX/GDI] Offscreen paths differ significantly from existing interactive window code** — need careful implementation, untested without Windows
- **[OpenGL] Shader path required** — user must supply `--shaders` or use compiled-in default. Exposes same failure mode as DrawMapOpenGL.
- **[Qt] QApplication lifecycle** — if Qt backend is present, `QApplication` is constructed even if user on headless system. Mitigation: wrap in `#ifdef` and ensure `QT_QPA_PLATFORM=offscreen` works.
- **[OSX] ObjC++** — file must be `.mm`, requires Apple toolchain. Conditional compilation with `__APPLE__`.
- **[Libpng errors]** Corrupt or empty images if libpng init fails. Mitigation: check all return values, print errors to stderr.
- **Backend not available at runtime** — build-time detection means user sees only compiled backends. OK, no runtime fallback needed.
