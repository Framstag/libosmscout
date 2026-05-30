## 1. Shared infrastructure

- [x] 1.1 Add `--shaders` option to DrawMap.h (for OpenGL shader path)
- [x] 1.2 Write `WriteRGBToPNG()` helper function in DrawMapAll.cpp using libpng (handles top-down and bottom-up row order)
- [x] 1.3 Add `mkdir -p` utility for output directory creation

## 2. CMake build system integration

- [x] 2.1 Add compute-time detection in Demos/CMakeLists.txt: collect available backends into `DRAWMAPALL_TARGETS` and `DRAWMAPALL_DEFS`
- [x] 2.2 Add `osmscout_demo_project(NAME DrawMapAll ...)` with conditional linkage and compile definitions
- [x] 2.3 Link libpng (`PNG::PNG`) when AGG or OpenGL backends are available

## 3. Meson build system integration

- [x] 3.1 Add `DrawMapAll` executable in Demos/meson.build with conditional deps per backend
- [x] 3.2 Add libpng dependency when needed

## 4. AGG backend block

- [x] 4.1 Allocate `agg::rendering_buffer` and `agg::pixfmt_rgb24` of size width×height
- [x] 4.2 Call `MapPainterAgg::DrawMap()` with pixel format
- [x] 4.3 Extract RGB buffer from `rendering_buffer::buf()` and write to `DrawMapAgg.png` via `WriteRGBToPNG()`

## 5. Cairo backend block

- [x] 5.1 Create `cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height)` and `cairo_t`
- [x] 5.2 Call `MapPainterCairo::DrawMap()` with cairo context
- [x] 5.3 Call `cairo_surface_write_to_png()` to save `DrawMapCairo.png`

## 6. OpenGL backend block

- [x] 6.1 Add GLFW initialization: hidden window creation, make context current
- [x] 6.2 Create `MapPainterOpenGL` with dimensions, DPI, font, shader path
- [x] 6.3 Call `ProcessData()`, `SwapData()`, `DrawMap()`
- [x] 6.4 Read buffer via `glReadPixels()`, Y-flip, write to `DrawMapOpenGL.png` via `WriteRGBToPNG()`
- [x] 6.5 Cleanup: destroy painter, GLFW window, terminate GLFW

## 7. Qt backend block

- [x] 7.1 Create minimal `QApplication` (argc, argv)
- [x] 7.2 Create `QPixmap(width, height)` and `QPainter`
- [x] 7.3 Call `MapPainterQt::DrawMap()` with painter
- [x] 7.4 Call `QPixmap::save("PNG")` to `DrawMapQt.png`
- [x] 7.5 Cleanup: delete painter and pixmap

## 8. SVG backend block

- [x] 8.1 Open `DrawMapSVG.svg` output file stream
- [x] 8.2 Create `MapPainterSVG` and call `DrawMap()` with stream
- [x] 8.3 Close stream

## 9. DirectX backend block (Windows only)

- [x] 9.1 Initialize COM via `CoInitializeEx()`
- [x] 9.2 Create `ID2D1Factory`, `IWICImagingFactory`, `IWICBitmap`, and WIC-backed `ID2D1RenderTarget`
- [x] 9.3 Create `MapPainterDirectX` and call `DrawMap()` with render target
- [x] 9.4 Save WIC bitmap as PNG via `IWICBitmapEncoder`
- [x] 9.5 Cleanup: release Direct2D/WIC resources, `CoUninitialize()`

## 10. GDI backend block (Windows only)

- [x] 10.1 Initialize GDI+ via `GdiplusStartup()`
- [x] 10.2 Create DIBSection with `CreateDIBSection()`, select into `CreateCompatibleDC()`
- [x] 10.3 Create `MapPainterGDI` and call `DrawMap()` with HDC
- [x] 10.4 Create `Gdiplus::Bitmap` from DIBSection HBITMAP and save PNG via `Gdiplus::Bitmap::Save()`
- [x] 10.5 Cleanup: delete GDI objects, `GdiplusShutdown()`

## 11. OSX backend block (macOS only)

- [x] 11.1 Create `CGBitmapContext` with ARGB pixel format
- [x] 11.2 Set as current `NSGraphicsContext`, apply Y-flip transform
- [x] 11.3 Call `MapPainterIOS::DrawMap()` with CGContextRef
- [x] 11.4 Extract `CGImage` from context, write PNG via `NSBitmapImageRep`
- [x] 11.5 Cleanup: release context, image, autorelease pool

## 12. Main function assembly

- [x] 12.1 Create `DrawMapAll.cpp` with `#ifdef` blocks for all backends
- [x] 12.2 Implement shared flow: parse args → OpenDatabase → LoadData → mkdir outputDir → dispatch backends
- [x] 12.3 Track per-backend success/failure, exit code reflects all failures

## 13. Verification

- [x] 13.1 Build with all backends enabled, verify compilation
- [x] 13.2 Build without a specific backend, verify conditional exclusion
- [x] 13.3 Run with test map data, verify all `.png`/`.svg` files exist and are valid
