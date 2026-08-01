# SVG Icon Loading — Design

## Overview

Add SVG icon support to `libosmscout-map-skia` with two code paths:

1. **Primary**: Skia's `SkSVGDOM` module — render SVG to offscreen canvas, snapshot to `SkImage`
2. **Fallback**: nanosvg (header-only C library, zlib license) — parse + rasterize to pixel buffer, wrap as `SkImage`

Both paths produce `sk_sp<SkImage>` stored in `iconCache`. `DrawIcon()` stays unchanged.

## SVG Loading Pipeline

```
HasIcon(style, projection, parameter)
    │
    ├── .png gefunden? → SkImages::DeferredFromEncodedData → cache (bestehend)
    │
    ├── .svg gefunden?
    │   │
    │   ├── #ifdef OSMSCOUT_HAVE_SKIA_SVG ─── SkSVGDOM path
    │   │   │
    │   │   ├── SkFILEStream → SkSVGDOM::Make(stream)
    │   │   ├── setContainerSize(width, height)  [bei Scalable Mode]
    │   │   ├── SkSurface::MakeRasterN32Premul(w, h)
    │   │   ├── svgDom->render(surface->getCanvas())
    │   │   └── surface->makeImageSnapshot() → cache
    │   │
    │   └── #else ─── nanosvg path
    │       │
    │       ├── nsvgParseFromFile(file, "px", 96.0f)
    │       ├── nsvgRasterize(...) → RGBA buffer
    │       └── SkImages::RasterFromData(info, data, stride) → cache
    │
    └── nichts → cache nullptr (bestehend)
```

## File Probing

Current code tries `path + name + ".png"`. Change to try both extensions:

```cpp
// Erst .png versuchen (bestehend)
std::string pngFile = path + style.GetIconName() + ".png";
// ... existing PNG loading ...

// Dann .svg versuchen (neu)
std::string svgFile = path + style.GetIconName() + ".svg";
// ... SVG loading (SkSVGDOM or nanosvg) ...
```

Order: PNG first (existing behavior preserved), SVG second.

## SkSVGDOM Path

### Include

```cpp
#include <svg/include/SkSVGDOM.h>
#include "include/core/SkStream.h"
```

Include path: add `SVG_INCLUDE_DIRS` (detected by CMake) to target includes.

### Render to SkImage

```cpp
SkFILEStream stream(svgFile.c_str());
if (!stream.isValid()) continue;

sk_sp<SkSVGDOM> svgDom = SkSVGDOM::Make(stream);
if (!svgDom) continue;

// Use native SVG size, or scale for Scalable mode
SkSize svgSize = svgDom->containerSize();
if (svgSize.isEmpty()) {
    // Fallback: get from root SVG element
    svgSize = SkSize::Make(svgDom->root()->intrinsicSize());
}
float scale = 1.0f;
if (parameter.GetIconMode() == MapParameter::IconMode::Scalable ||
    parameter.GetIconMode() == MapParameter::IconMode::ScaledPixmap) {
    float targetSize = style.GetWidth();
    scale = targetSize / std::max(svgSize.width(), svgSize.height());
    svgSize = SkSize::Make(svgSize.width() * scale, svgSize.height() * scale);
}
svgDom->setContainerSize(svgSize);

// Render to offscreen surface
auto surface = SkSurface::MakeRasterN32Premul(
    (int)svgSize.width(), (int)svgSize.height());
if (!surface) continue;
svgDom->render(surface->getCanvas());

sk_sp<SkImage> image = surface->makeImageSnapshot();
iconCache[style.GetIconName()] = image;
```

## nanosvg Path

### Include

```cpp
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
```

Vendored header at `libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h`.

### Rasterize to SkImage

```cpp
NSVGimage* svgImage = nsvgParseFromFile(svgFile.c_str(), "px", 96.0f);
if (!svgImage) continue;

float scale = 1.0f;
int w = (int)svgImage->width;
int h = (int)svgImage->height;
if (parameter.GetIconMode() == MapParameter::IconMode::Scalable ||
    parameter.GetIconMode() == MapParameter::IconMode::ScaledPixmap) {
    float targetSize = style.GetWidth();
    scale = targetSize / std::max(w, h);
    w = (int)(w * scale);
    h = (int)(h * scale);
}

unsigned char* rgba = (unsigned char*)malloc(w * h * 4);
nsvgRasterize(svgImage, 0, 0, scale, rgba, w, h, w * 4);

sk_sp<SkData> pixelData = SkData::MakeFromMalloc(rgba, w * h * 4);
SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
sk_sp<SkImage> image = SkImages::RasterFromData(info, pixelData, w * 4);

nsvgDelete(svgImage);
iconCache[style.GetIconName()] = image;
```

## CMake Detection

### features.cmake

Add probe for Skia SVG module after Skia core is found:

```cmake
# Probe for Skia SVG module
set(OSMSCOUT_HAVE_SKIA_SVG OFF)
if(SKIA_FOUND)
  # Check for SkSVGDOM header
  find_path(SVG_INCLUDE_DIRS
    NAMES svg/include/SkSVGDOM.h
    HINTS ${SKIA_INCLUDE_DIRS} /usr/include/svg /usr/local/include/svg
  )
  if(SVG_INCLUDE_DIRS)
    # Check for SVG symbols in libskia
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_LIBRARIES ${SKIA_LIBRARY})
    check_symbol_exists("_ZNK7SkSVGDOM4rootEv" "${SKIA_LIBRARY}" SKIA_SVG_SYMBOLS)
    if(SKIA_SVG_SYMBOLS)
      set(OSMSCOUT_HAVE_SKIA_SVG ON)
    endif()
  endif()
endif()
```

### MapSkiaFeatures.h.cmake

```cmake
#cmakedefine OSMSCOUT_HAVE_SKIA_SVG
```

### MapPainterSkia.cpp

```cpp
#include <osmscoutmapskia/MapSkiaFeatures.h>

#ifdef OSMSCOUT_HAVE_SKIA_SVG
  #include <svg/include/SkSVGDOM.h>
  #include "include/core/SkStream.h"
  #include "include/core/SkSurface.h"
#else
  #define NANOSVG_IMPLEMENTATION
  #include "nanosvg.h"
#endif
```

### libosmscout-map-skia/CMakeLists.txt

```cmake
if(OSMSCOUT_HAVE_SKIA_SVG)
  target_include_directories(OSMScoutMapSkia PRIVATE ${SVG_INCLUDE_DIRS})
  target_compile_definitions(OSMScoutMapSkia PRIVATE OSMSCOUT_HAVE_SKIA_SVG)
endif()
```

## Icon Scaling

| Mode | SkSVGDOM | nanosvg |
|------|----------|---------|
| `OriginalPixmap` | Native SVG size | Native SVG size |
| `Scalable` / `ScaledPixmap` | `setContainerSize(scaled)` + render | `nsvgRasterize(scale)` |

## Files Changed

| File | Change |
|------|--------|
| `cmake/features.cmake` | Add Skia SVG module detection (`SVG_INCLUDE_DIRS`, `OSMSCOUT_HAVE_SKIA_SVG`) |
| `libosmscout-map-skia/include/osmscoutmapskia/MapSkiaFeatures.h.cmake` | Add `#cmakedefine OSMSCOUT_HAVE_SKIA_SVG` |
| `libosmscout-map-skia/CMakeLists.txt` | Add `SVG_INCLUDE_DIRS` and conditional define |
| `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` | Extend `HasIcon()` with SVG probing + dual rendering paths |
| `libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h` | New — vendored nanosvg header (fallback path) |

## Not Changing

- `DrawIcon()` — same `sk_sp<SkImage>` cache lookup
- `MapPainterSkia.h` — no new members or methods
- `iconCache` type — stays `std::map<std::string, sk_sp<SkImage>>`
- `StyleSheetChanged()` — already clears `iconCache`, covers both paths
- Other backends or core library

## Edge Cases

| Case | Behavior |
|------|----------|
| `.svg` file missing | Falls through, cache nullptr (same as missing PNG) |
| Malformed SVG | Both parsers return nullptr, falls through |
| SkSVGDOM unavailable at build | nanosvg fallback compiled in |
| Both `.png` and `.svg` exist | PNG wins (probed first) |
| Zero-size SVG | SkSVGDOM: empty surface; nanosvg: empty buffer |
| Very large SVG | Scaled down in Scalable mode; native size in OriginalPixmap |
