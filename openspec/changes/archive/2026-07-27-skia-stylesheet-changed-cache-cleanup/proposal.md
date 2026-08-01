## Why

`MapPainterSkia` does not override `StyleSheetChanged`, so icon and pattern caches survive style reloads. Users who switch stylesheets mid-session see stale icons from the previous style — a correctness bug. Cairo and Qt backends both clear their caches in this callback.

## What Changes

1. Add `StyleSheetChanged` override to `MapPainterSkia` (declaration in header, implementation in `.cpp`)
2. Clear `iconCache` (`std::map<std::string, sk_sp<SkImage>>`) on style change
3. Clear `patternCache` (`std::map<std::string, sk_sp<SkShader>>`) on style change
4. Update `libosmscout-map-skia/TODO.md` to mark `StyleSheetChanged` cache cleanup as complete

## Capabilities

### New Capabilities

*(none — this change modifies an existing capability)*

### Modified Capabilities

- `map-painter-skia`: Add `StyleSheetChanged(const Projection&, const MapParameter&, const std::vector<MapData>&)` override that clears icon and pattern caches, matching the pattern established by `MapPainterCairo::StyleSheetChanged` and `MapPainterQt::StyleSheetChanged`.

## Impact

- **Modified files**: `libosmscout-map-skia/include/osmscoutmapskia/MapPainterSkia.h`, `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp`, `libosmscout-map-skia/TODO.md`
- **API**: No public API change — `StyleSheetChanged` is a virtual method on `MapPainter` base class, already called by the framework. This is purely an override addition.
- **Dependencies**: None. Uses existing Skia smart pointers (`sk_sp`) which auto-release on `map::clear()`.
- **Build**: Both CMake and Meson pick up the new method automatically (no file list changes needed).
