## Context

JavaScout is a Java demo app in `JavaScout/`. It depends on `libosmscout-client-java` which produces:
- `libosmscoutclientjava.jar` — Java bytecode (OSMScoutClient, OSMScoutClientBuilder)
- `libosmscout_client_java.so` — native JNI shared library (C++)

The native library is built by Meson (C++ compilation). The jar is also built by Meson currently. The proposal keeps Meson for the native part and moves only the JavaScout app to Maven.

### Current Build Flow

```
root meson.build
  ├── subdir('libosmscout-client-java')
  │     ├── jar('libosmscoutclientjava')     ← Java bytecode
  │     └── library('osmscout_client_java')   ← C++ JNI .so
  └── subdir('JavaScout')
        └── jar('JavaScout')  ← link_with: osmscoutclientjavaJar
```

### Proposed Build Flow

```
meson setup build && meson compile -C build    ← builds native .so + .jar
cd JavaScout && mvn package                     ← builds JavaScout app
```

The bridge: `maven-install-plugin` installs the Meson-built jar into local Maven repo during `initialize` phase, then JavaScout depends on it as a normal Maven dependency.

## Goals / Non-Goals

**Goals:**
- JavaScout builds with `mvn package`
- JavaScout can declare Maven dependencies (Swing, etc.) normally
- Build is configurable for different Meson/CMake build directories
- Root Meson build no longer builds JavaScout (but still builds client-java)
- Clear documentation for developers

**Non-Goals:**
- Changing how `libosmscout-client-java` is built (stays Meson/CMake)
- Publishing artifacts to remote Maven repositories
- Android build setup (separate future effort)
- Changing the native JNI loading mechanism

## Decisions

1. **Maven over Gradle for JavaScout**
   - Simpler for a single-module Java app
   - Android will use Gradle (standard), no need to unify
   - Maven's `install:install-file` handles local pre-built deps cleanly

2. **`system` scope over `install:install-file`**
   - `install:install-file` runs in `initialize` phase, but Maven resolves dependencies before that phase — timing mismatch
   - `system` scope resolves directly from filesystem path, no repository needed
   - Works with IDE import, no `~/.m2/repository` pollution
   - Trade-off: `system` scope is technically deprecated (but widely used for local deps, no replacement exists for this use case)

3. **`-Dclient-java.jar=/absolute/path` for custom paths**
   - Build directory is user-configurable (`build/`, `debug/`, `build-meson/`, etc.)
   - `systemPath` requires absolute path — `-D` flag with absolute path is the override mechanism
   - Default path uses `${project.basedir}` (always absolute) for standard Meson `build/` dir
   - Env var approach rejected: Maven can't convert relative env var values to absolute paths

4. **Remove JavaScout from root Meson build**
   - Avoids confusion (two build systems claiming the same target)
   - Root Meson build still builds client-java (needed by both JavaScout and C++ consumers)
   - Conditional skip via `if` guard in root `meson.build` for backward compat

5. **`libosmscout-client-java` is Meson-only**
   - No CMakeLists.txt exists for the Java client library
   - Users who build C++ libs with CMake still need a separate Meson build for the jar
   - Documented clearly in README
   - Future option: add CMake support to client-java (separate change)

6. **Fat jar via `maven-shade-plugin`**
   - `maven-assembly-plugin` excludes `system` scope deps — shade plugin includes them with `<includeSystemScope>true</includeSystemScope>`
   - Shade plugin replaces the main jar — `target/javascout-1.0-SNAPSHOT.jar` is the fat jar
   - Run with `java -Djava.library.path=... -jar javascout-1.0-SNAPSHOT.jar`
   - No classpath hassle — only `-Djava.library.path` needed for native `.so`

## Runtime Native Library Loading

The native `.so`/`.dylib` is loaded at runtime via `java.library.path`. This is unchanged from current behavior:

```bash
java --enable-native-access=ALL-UNNAMED \
     -cp JavaScout.jar:libosmscoutclientjava.jar \
     -Djava.library.path=../build/libosmscout-client-java/src \
     com.framstag.libosmscout.JavaScout \
     /path/to/map/dir
```

A launcher script (`JavaScout/javascout.sh`) wraps this for convenience. It prefers the fat jar (`-jar-with-dependencies.jar`) and falls back to thin jar + classpath.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| `install:install-file` fails if Meson hasn't built client-java first | Document build order; `initialize` phase fails fast with clear error |
| Path to jar is fragile across platforms | Env var override + documented default |
| IDE import of Maven project won't know about Meson step | Document in README; CI scripts handle both steps |
| `~/.m2/repository` pollution from SNAPSHOT installs | Standard Maven behavior; `mvn dependency:purge-local-repository` if needed |
| Future Android client needs different dep format | Android gets separate Gradle build; can consume same jar or AAR wrapper |
