## Why

JavaScout currently does not display POI icons on the map because the native client is not supplied with an icon directory. The builder accepts an icon path in the Java bindings, but JavaScout does not expose a configurable icon path and falls back to a hard-coded SVG-only directory that does not contain the full icon set used by the standard stylesheet. This makes the map look bare and reduces the usefulness of the application.

## What Changes

- Add an `--icon-dir` command-line option to JavaScout (default: `libosmscout/data/icons/14x14/standard/`).
- Persist the configured icon directory in `config.properties` so the setting survives restarts.
- Migrate a persisted old SVG default (`libosmscout/data/icons/svg/standard`) to the PNG default used by the Cairo renderer.
- Pass the configured icon directory to `OSMScoutClientBuilder.withIconDirectory()` during client initialisation.
- Remove the hard-coded icon path from `MainController`.
- Ensure `Config` provides read/write access for the icon directory key.
- Update `JavaScout` argument parsing to forward the icon directory to `JavaScoutApp`.
- Add unit tests for `Config` icon directory persistence and migration.
- Update `javascout.sh` to supply an absolute default icon directory and log the chosen native library path.
- `JavaScout/javascout.sh` — supply absolute default icon directory and log the chosen native library path.

## Capabilities

### New Capabilities
- `javascout-icon-config`: Configure and persist the icon directory used by the JavaScout map renderer.

### Modified Capabilities
- `map-rendering`: Extend the rendering capability to require that the renderer is initialised with a valid icon directory so POI icons can be drawn.

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/JavaScout.java` — parse `--icon-dir`.
- `JavaScout/src/main/java/com/framstag/libosmscout/JavaScoutApp.java` — forward icon directory to the controller.
- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — use configured icon path, remove hard-coded value.
- `JavaScout/src/main/java/com/framstag/libosmscout/Config.java` — add icon directory property.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — pass the configured icon directory to `MapParameter::SetIconPaths()` before rendering.
- `libosmscout-client/include/osmscoutclient/DBThread.h` — expose `GetIconDirectory()` so the JNI render path can read the directory stored during client construction.
- `JavaScout/javascout.sh` — supply absolute default icon directory and log the chosen native library path.
- `libosmscout-client-java` Java and C++ layers already support icon directory via `OSMScoutClientBuilder`; no changes required there.
