## Why

JavaScout currently uses a Meson build. As the app gains more Java dependencies (Swing, etc.), Meson becomes unsuitable — it's a C/C++ build system with limited Java dependency management. Maven is the standard Java build tool with mature dependency resolution, lifecycle management, and IDE support.

The native C++ JNI layer (`libosmscout-client-java`) stays with Meson/CMake — that's C++ code and Meson handles it well. JavaScout only needs the `.jar` output from that build.

## What Changes

- Replace `JavaScout/meson.build` with `JavaScout/pom.xml`
- Remove `JavaScout` from Meson's root `meson.build` subdir list
- Add `maven-install-plugin` to install the pre-built `libosmscoutclientjava.jar` into local Maven repo during `initialize` phase
- Add profile for `CLIENT_JAVA_JAR` env var to override default jar path
- Add `JavaScout/pom.xml` with:
  - Normal Maven project structure
  - Dependency on `libosmscout-client-java` (installed from Meson output)
  - Plugin config for `maven-install-plugin` and `maven-jar-plugin`
  - Properties for build flexibility
- Update root `meson.build` to conditionally skip JavaScout subdir
- Add `JavaScout/README.md` with build instructions

## Capabilities

### New Capabilities
- `javascout-maven-build`: Maven-based build for JavaScout with configurable client-java jar path

### Modified Capabilities
*(none — no runtime behavior changes, build system only)*

## Impact

- Build system only (JavaScout + root meson.build)
- No API/ABI changes
- No runtime behavior changes
- Existing Meson-based JavaScout build stops working (replaced)
- Root Meson build no longer builds JavaScout automatically
- CI workflows that build JavaScout need updating
