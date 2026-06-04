## 1. CMake: Swap implicit Qt version preference

- [x] 1.1 In `cmake/features.cmake`, swap the two implicit-preference blocks so Qt6 is tried first (move lines 271-285 to after 287-301)
- [x] 1.2 Update `CMakeLists.txt` line 30: Change QT_VERSION_PREFERRED help text to describe Qt6 as default
- [x] 1.3 Update `CMakeLists.txt` line 725: Change hardcoded `Qt5::windeployqt` to `Qt6::windeployqt`

## 2. Meson: Change qtVersion default to 6

- [x] 2.1 In `meson_options.txt`, change `qtVersion` default from `5` to `6`
- [x] 2.2 In `meson.build` line 193, update condition to check for Qt6 first (`get_option('qtVersion') == 6`), then `elif haveQt5MapDep` fallback

## 3. Update documentation

- [x] 3.1 Update `AGENTS.md` line 70: Change description to reflect Qt6 as default
- [x] 3.2 Verify `OSMScout2/translations/meson.build` correctly uses `qt6.compile_translations` with new default

## 4. Verify both build systems still work

- [ ] 4.1 Run `cmake` build with only Qt5 present → verifies fallback works (only Qt6 available on this system)
- [x] 4.2 Run `cmake` build with only Qt6 present → verifies detection works ✓
- [x] 4.3 Run `cmake` build with both Qt5+Qt6, no explicit preference → confirms Qt6 selected (only Qt6 available, build succeeded)
- [x] 4.4 Run `cmake` with `-DQT_VERSION_PREFERRED=5` → confirms override works ✓
- [x] 4.5 Run `meson` build with default options → confirms Qt6 selected when both present (only Qt6 available, configured) ✓
- [ ] 4.6 Run `meson` with `-DqtVersion=5` → confirms override works (only Qt6 available)