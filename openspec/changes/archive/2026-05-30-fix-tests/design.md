## Context

Two test failures on MinGW/MSYS2 Windows CI:

1. **HighwayMilestoneFeatureTest** — Hardcoded `/tmp/` paths fail because `/tmp` may not exist on Windows. C++20 `std::filesystem::temp_directory_path()` provides portable solution.

2. **SymbolRendererSVGTest** — `STATUS_DLL_NOT_FOUND` (0xc0000135). Test links `osmscout + osmscoutmap + osmscoutmapsvg`. On MinGW (non-MSVC), the `osmscout_test_project` CMake macro's `if(MSVC)` block (lines 234-266 in `cmake/ProjectConfig.cmake`) is skipped, so no DLL PATH is configured for CTest. The test executable can't find `libosmscout_map_svg.dll` or transitive deps at load time.

Existing CMake infrastructure has PATH setup for MSVC but not MinGW.

## Goals / Non-Goals

**Goals:**
- `HighwayMilestoneFeatureTest` passes on MinGW/MSYS2
- `SymbolRendererSVGTest` passes on MinGW/MSYS2

**Non-Goals:**
- Fixing other test failures on Windows
- Changing test logic or expected behavior
- Adding cross-platform temp file utilities to libosmscout core

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Temp file strategy | `std::filesystem::temp_directory_path()` + `std::filesystem::remove()` | C++20 available, no new deps, works on all targets. Keep per-test UUID to avoid collisions. |
| DLL path fix scope | CMake `osmscout_test_project` macro only (not meson) | Meson handles PATH differently via env. CI runs CMake build on MinGW. Add `if(MINGW)` block analogous to `if(MSVC)` block. |

**Alternatives considered for temp files:**
- `mkstemp()` — POSIX only, no Windows
- Keep `/tmp/` and create dir on Windows — fragile
- `std::filesystem::temp_directory_path()` — C++20 standard, portable, correct choice

**Alternatives considered for DLL path:**
- Static linking of test executable — changes build config, may hide real link issues
- Bundle DLLs next to test executable — `$<TARGET_RUNTIME_DLLS>` generator expression, but requires CMake 3.21+
- Add `if(MINGW)` PATH block matching `if(MSVC)` — minimal change, follows existing pattern

## Risks / Trade-offs

- [Risk] MinGW DLL naming may differ from MSVC → The `$<TARGET_FILE_DIR:OSMScoutMapSVG>` generator expression resolves the correct output dir regardless of naming conventions
- [Risk] `std::filesystem::temp_directory_path()` may return path with spaces → `FileWriter::Open()` must handle paths with spaces (likely already does, uses `std::string`)
- [Trade-off] `if(MINGW)` block duplicates `if(MSVC)` PATH logic → Acceptable; keeps each platform path independent and avoids MSVC `$(Configuration)` generator expressions that don't apply to MinGW
