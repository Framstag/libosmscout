## Why

Qt5 is EOL since November 2020 and increasingly unavailable in modern distributions. Upstream libosmscout already supports Qt6 fully (all components, Core5Compat, etc.). The build system still defaults to Qt5 when both are present, forcing users to set -DQT_VERSION_PREFERRED=6 (CMake) or -DqtVersion=6 (Meson) to use the modern version.

## What Changes

- CMake (cmake/features.cmake): Swap implicit Qt version preference order. Try Qt6 first when no explicit QT_VERSION_PREFERRED set, fall back to Qt5.
- Meson (meson_options.txt): Change qtVersion default from 5 to 6.
- Meson (meson.build): Update qtVersion selection logic to match new default. Qt6 tried first, Qt5 fallback.
- Update option descriptions in CMakeLists.txt, meson_options.txt, AGENTS.md to reflect Qt6 as default.
- No changes needed for single-version-only environments. Existing fallback logic handles that.

## Capabilities

### New Capabilities

- (none - this is a build config change, no new spec)

### Modified Capabilities

- (none - no spec-level behavior changes, only build system default preference)

## Impact

- CMake (cmake/features.cmake): Swap implicit Qt preference blocks (lines 271-301). Qt6 tried before Qt5.
- Meson (meson_options.txt): Change qtVersion default value to 6.
- Meson (meson.build): Update qtVersion selection logic (line 193). Try Qt6 first, Qt5 fallback.
- CMakeLists.txt (line 30): Update QT_VERSION_PREFERRED help text.
- AGENTS.md: Update Qt version selection description.
- All downstream builds without explicit QT_VERSION_PREFERRED (CMake) or -DqtVersion=5 (Meson) will now default to Qt6.
- Explicit QT_VERSION_PREFERRED=5 (CMake) or -DqtVersion=5 (Meson) continues to work unchanged.