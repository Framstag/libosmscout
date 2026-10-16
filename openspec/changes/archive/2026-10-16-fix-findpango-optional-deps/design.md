# Design

## Root cause

`cmake/FindPango.cmake` decides what to report, and its pangoft2 branch additionally decides what the
project *requires*: `FIND_PACKAGE(Freetype REQUIRED)` and `FIND_PACKAGE(Fontconfig REQUIRED)` inside
`IF(PANGOFT2_INCLUDE_DIRS)` - lines 112 and 116 of the file as the failing configure read it. CMake
therefore refuses any configure that turns
either package off - `FIND_PACKAGE for module <name> called with REQUIRED, but
CMAKE_DISABLE_FIND_PACKAGE_<name> is enabled` - and the refusal is not a property of the backend but of
two lines of a find module whose consumers treat both packages as optional.

The two calls are not load bearing for anything in the file: each is followed by an
`IF(<NAME>_FOUND)` that appends include directories. `cmake/features.cmake` reaches the module through `include(...)` at `:257` and calls `find_package` for
both on its own (`:227`, `:264`), derives `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` (`:271`) and
`OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` (`:272`) from those results, not from this module. So dropping
the keyword changes the failure mode only.

## The single fix

Remove `REQUIRED` from both calls and state in a comment why neither may be required. The
`IF(FREETYPE_FOUND)` / `IF(FONTCONFIG_FOUND)` guards already handle a package that is absent or
disabled: the include directories are simply not appended, exactly as they are not appended today on a
host that lacks the package. No variable, no guard, no `#cmakedefine` and no `HANDLE_STANDARD_ARGS`
result changes.

```cmake
        FIND_PACKAGE(Freetype)
        IF(FREETYPE_FOUND)
            SET(PANGOFT2_INCLUDE_DIRS ${PANGOFT2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
        ENDIF()
        FIND_PACKAGE(Fontconfig)
        IF(FONTCONFIG_FOUND)
            SET(PANGOFT2_INCLUDE_DIRS ${PANGOFT2_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})
        ENDIF()
```

Blast radius: the two lines, plus a three-line comment. No compiled file, no configured template, no
public header. `PANGOFT2_INCLUDE_DIRS` keeps the same value on a host that provides both packages, and
gains no value on a host that provides neither - an inference from the two guards and the two lines they
follow, not a retained measurement: no pre-fix default configure was kept, and what the retained default
configure shows is the guards' result (it finds both packages and defines both feature gates), not the
value of the variable itself.

Rollback: re-add the `REQUIRED` keywords.

## Verified behaviour

The configure matrix in `tasks.md` and `evidence/verification.md`:

| run | dependency disabled | result |
|---|---|---|
| a | FreeType | configure exit 0 (`-DOSMSCOUT_BUILD_MAP_OPENGL=OFF`, see below) |
| b | fontconfig | configure exit 0 |
| c | none | configure exit 0, both found, both feature gates defined |
| d | FreeType / fontconfig | `OSMScoutMapCairo` builds in both, no warning |

`CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` also switches off the **OpenGL map backend**, which requires
FreeType for its text loader (`CMakeLists.txt:341` reports `freetype2 is required for OpenGL map drawing
backend`, `libosmscout-map-opengl/src/osmscoutmapopengl/TextLoader.cpp:31`). That is a real requirement of
that backend, not a feature-gated optional dependency, so run (a) disables the OpenGL backend explicitly
and the requirement stays as it is. Without `-DOSMSCOUT_BUILD_MAP_OPENGL=OFF` that configure still fails
by design; the failure no longer names `FindPango.cmake`.

## Alternatives the evidence eliminated

1. **An option that decides whether the font dependencies are required** (e.g.
   `OSMSCOUT_MAP_CAIRO_REQUIRE_FONT_DEPS`). Eliminated: `AGENTS.md` ("Dependencies") and
   `cmake/features.cmake` already answer the question - both packages are optional for
   `libosmscout-map` and for the Cairo backend and are feature-gated. Offering the choice back would
   contradict a recorded decision, and two viable policies for one line is exactly the shape condition
   3 rejects.
2. **Drop the two `FIND_PACKAGE` calls and take the include directories from `features.cmake`'s calls
   instead.** Eliminated: the file publishes `PANGOFT2_INCLUDE_DIRS` and would silently lose the
   FreeType/fontconfig include directories from it; the larger delta buys nothing the keyword removal
   does not.
3. **Port the module to `PkgConfig::PANGOFT2` imported targets.** Eliminated: a find-mechanism rewrite
   for a defect that is one keyword on two lines; condition 5 bounds the delta and the module's
   `PKG_CHECK_MODULES` + `FIND_PATH` shape is not the cause.
4. **Report `PANGOFT2_FOUND` false when either package is missing.** Eliminated: no evidence asks for
   the Cairo font paths to be switched off by a dependency they only optionally use, and it would be a
   behaviour change beyond the defect.
5. **Leave the calls but disable them from `CMAKE_DISABLE_FIND_PACKAGE_*`.** Eliminated: CMake rejects
   that combination by design; there is no supported way to override it.

## Risk

- Low. The delta removes a constraint rather than adding one, and the only changed behaviour is the
  configure outcome with a package disabled - a configuration that could not be configured before.
- A host that has pangoft2 but neither FreeType nor fontconfig is now configurable, and the Cairo
  backend falls back exactly as its feature gates prescribe. That path is what the two builds of run (d)
  compile.
- Residual: the "the file parses and the module still finds both packages when present" claim rests on
  the default configure of run (c), which is part of the matrix and not a separate check.
