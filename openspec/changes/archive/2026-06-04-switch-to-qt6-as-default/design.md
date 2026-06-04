## Context

libosmscout supports both CMake and Meson build systems. Current Qt version selection logic defaults to Qt5 when both Qt5 and Qt6 are detected:

**CMake** (`cmake/features.cmake`, lines 243-301): When no explicit `QT_VERSION_PREFERRED` is set, the implicit path tries Qt5 first (line 271-274), then Qt6 only as fallback (lines 287-290).

**Meson** (`meson.build`, lines 193-213): `get_option('qtVersion')` defaults to `5` via `meson_options.txt` (line 16), so Qt5 checked first. Qt6 only reached via `elif haveQt6MapDep` (line 203).

Both `AGENTS.md` and `CMakeLists.txt` describe Qt5 as the default.

Additionally, `CMakeLists.txt` line 725 has a hardcoded `Qt5::windeployqt` reference.

## Goals / Non-Goals

**Goals:**
- Swap default preference from Qt5 to Qt6 when both versions are present
- Preserve explicit `QT_VERSION_PREFERRED=5` (CMake) / `-DqtVersion=5` (Meson) override
- No behavioral change for systems with only one Qt version installed
- Update documentation references

**Non-Goals:**
- Removing Qt5 support entirely
- Changing Qt5-specific code paths or compatibility shims (e.g., Core5Compat)

## Decisions

### CMake implicit preference order swap

**Decision**: In `cmake/features.cmake`, swap the two implicit blocks. The Qt5-first implicit block (lines 271-285) becomes Qt6-first, and the Qt6-fallback block (lines 287-301) becomes Qt5-fallback.

**Rationale**: Minimal diff. The explicit-preference blocks (lines 243-269, gated by `QT_VERSION_PREFERRED EQUAL 5/6`) stay unchanged since those handle user overrides. Only the implicit/no-preference path changes order.

### Meson qtVersion default change

**Decision**: Change `meson_options.txt` default from `5` to `6`, and update `meson.build` logic to match.

**Rationale**: The Meson logic (line 193) checks `get_option('qtVersion') == 5` first, then `elif haveQt6MapDep`. With the default changed to `6`, this becomes `get_option('qtVersion') == 6` first, `elif haveQt5MapDep` fallback. No structural changes needed beyond swapping the conditions.

### Hardcoded Qt5::windeployqt reference

**Decision**: Update `CMakeLists.txt` line 725 from `Qt5::windeployqt` to `Qt6::windeployqt`.

**Rationale**: With Qt6 default, the windeployqt target name should match. Note: this only fires on Windows when Qt DLL install is enabled.

## Risks / Trade-offs

- **[Risk] CI systems with only Qt5** → Existing Qt5 fallback path still works. CI configs setting `-DQT_VERSION_PREFERRED=5` or `-DqtVersion=5` unchanged.
- **[Risk] OSMScout2/translations/meson.build** → Uses `get_option('qtVersion')` directly. With default `6`, it will use `qt6.compile_translations()`. This is correct.
- **[Risk] Downstream consumers relying on CMake's qt5-first search** → They should use explicit `QT_VERSION_PREFERRED=5`. This is a build-config default change, not an API change.
- **[Minimal risk]** No runtime behavioral changes. Only affects build-time Qt version selection.