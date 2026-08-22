## 1. StyleConfig Symbol Enumeration API

- [x] 1.1 Add public `GetSymbolNames()` returning sorted `std::vector<std::string>` to `libosmscout-map/include/osmscoutmap/StyleConfig.h` (next to `GetSymbol`) and implement in `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` by copying the keys of the private `symbols` map and sorting them. Verify: file compiles; method returns names of a stylesheet with known symbols (spec: Symbol enumeration API).
- [x] 1.2 Add unit test `Tests/src/StyleConfigSymbolsTest.cpp` registered in `Tests/CMakeLists.txt` + `Tests/meson.build` covering: enumeration of a stylesheet with two known symbols returns exactly those two; empty stylesheet returns empty list. Verify: `ctest -R StyleConfigSymbols` passes (spec: Symbol enumeration API scenarios).
- [x] 1.3 Run full test suite and verify no regressions: `cd build && ctest -j 2 --output-on-failure` (rule: existing tests still pass).

## 2. SymbolsAll Tool Skeleton and CLI

- [x] 2.1 Create `Demos/src/SymbolsAll.cpp` with `main`, argument parser derived from `osmscout::CmdLineParser` (following `TextMetricsAll.cpp`): options `--stylesheet <file.oss>` (required), `--output <dir>` (default `symbols-output`), `--dpi` (default 96), `--size <n>` square canvas (default 256), `--backend cairo|svg|all`, `--ost <file>` (optional), `--list`, `--sheet`. Verify: `--help` prints all options; missing `--stylesheet` prints error and exits non-zero (spec: Load stylesheet for symbol scanning; design D9).
- [x] 2.2 Implement type configuration resolution: `--ost` if given, else sibling `<stylesheet>.ost`, else `<stylesheet-dir>/map.ost`; error + exit code 1 if none found. Verify: run against `stylesheets/motorways.oss` with `motorways.ost` present and with the ost removed (exit non-zero) (spec: Type definition file resolution).
- [x] 2.3 Implement stylesheet loading: `TypeConfig::LoadFromOSTFile` + `StyleConfig(typeConfig).Load(stylesheet)`; non-existent stylesheet → error message + non-zero exit. Verify: run with bogus path, observe exit code 1 (spec: Load stylesheet for symbol scanning).

## 3. Cairo Backend Rendering

- [x] 3.1 Implement Cairo render path (guarded by `#if defined(HAVE_OSMSCOUT_MAP_CAIRO)`): per symbol create `cairo_image_surface_create(CAIRO_FORMAT_RGB24, N, N)`, paint white, compute auto-fit `scaleFactor` from `Symbol::GetBoundingBox(projection)` + `GetMaxBorderWidth` (design D4), instantiate `SymbolRendererCairo` and call `Render(projection, symbol, Vertex2D(N/2,N/2), scale)`, write PNG to `<output>/<sanitized-name>.png` (sanitize name to `[A-Za-z0-9_-]`); create output dir with `std::filesystem::create_directories`; per-symbol stdout line. Verify: run on `stylesheets/motorways.oss` → `place_city.png` etc. exist and open (spec: Render all symbols via Cairo backend; Failure reporting).
- [x] 3.2 Handle Cairo-unavailable case: `--backend cairo` with no Cairo support prints error, exit non-zero (spec: Cairo backend unavailable scenario).
- [x] 3.3 Record render/write failures per symbol, exit code 2 if any failed. Verify: force write failure (unwritable output path), expect exit 2 (spec: Failure reporting).

## 4. SVG Backend Rendering

- [x] 4.1 Render SVG path (guarded by `#if defined(HAVE_OSMSCOUT_MAP_SVG)`): for each symbol open `std::ofstream`, write SVG document header (`<?xml?>`, `<svg xmlns width height viewBox="0 0 N N">`, white background `<rect>`), construct `SymbolRendererSVG(stream)`, call `Render(...)` with same projection/fit as 3.1, close `</svg>`, write `<output>/<name>.svg`. Verify: files exist and parse as XML (e.g. `xmllint --noout`) (spec: Render all symbols to SVG; design D5).
- [x] 4.2 Both backends in one run: `--backend all` renders both PNG + SVG per symbol. Verify: run with `all`, both file sets present (spec: Render all symbols in a single run).

## 5. List Mode and Contact Sheet

- [x] 5.1 `--list` mode: load stylesheet, print each symbol name one per line, exit 0, write no image files. Verify: output equals sorted `GetSymbolNames()` output, no files created (spec: List symbol names without rendering).
- [x] 5.2 Cairo contact sheet (`--sheet`): single PNG grid, `ceil(sqrt(n))` columns, symbol name below each cell (design D7), named `symbols.png`. Verify: file exists with expected grid dimension (spec: Contact sheet output for Cairo).
- [x] 5.3 SVG contact sheet: single `symbols.svg` with `<text>` labels per cell. Verify: file exists, valid XML (spec: Contact sheet for SVG).

## 6. Build Wiring

- [x] 6.1 Register `SymbolsAll` in `Demos/CMakeLists.txt` following the `TextMetricsAll` block: `osmscout_demo_project(NAME SymbolsAll ...)`, link `PNG::PNG` when Cairo, `HAVE_OSMSCOUT_MAP_CAIRO`/`HAVE_OSMSCOUT_MAP_SVG` compile defs per backend. Verify: `cmake --build build` compiles `SymbolsAll` target (rule: build compiles without errors).
- [x] 6.2 Register `SymbolsAll` in `Demos/meson.build` following `TextMetricsAll`/`DrawMapAll` pattern with same backend conditionals. Verify: `meson compile -C build` succeeds (rule: both build systems per AGENTS.md pitfalls).
- [x] 6.3 Uncrustify format new/changed files per `.uncrustify` config. Verify: `uncrustify -c .uncrustify --check` on `Demos/src/SymbolsAll.cpp` reports clean (rule: formatting).

## 7. End-to-End Verification

- [x] 7.1 Run tool against all stylesheets in `stylesheets/` (`standard.oss`, `motorways.oss`, `cycle.oss`, `railways.oss`, `winter-sports.oss`, `public-transport.oss`, `boundaries.oss`) with `--backend all`, verify per-symbol PNG/SVG produced without exit code 1/2; spot-check a few images visually. Verify: exit 0 for each stylesheet (spec: Render all backends; integration).
- [x] 7.2 Build with ASan/UBSan config per AGENTS.md and run `SymbolsAll` on `motorways.oss`. Verify: no sanitizer reports (rule: no memory errors).
