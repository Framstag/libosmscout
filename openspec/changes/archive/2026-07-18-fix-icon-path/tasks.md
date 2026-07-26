## 1. Config support for icon directory

- [x] 1.1 Add `icon.directory` default constant `libosmscout/data/icons/14x14/standard/` in `Config` (spec: javascout-icon-config, 1 SP).
- [x] 1.2 Add `Config.getIconDirectory()` returning the configured value, the default, or the migrated PNG default (spec: javascout-icon-config, 1 SP).
- [x] 1.3 Add `Config.setIconDirectory(String)` writing to `icon.directory` (spec: javascout-icon-config, 1 SP).
- [x] 1.4 Add unit tests in `ConfigTest.java` covering default value, explicit set/get, persistence across save/load, and legacy SVG migration (spec: javascout-icon-config, 2 SP).

## 2. CLI argument parsing

- [x] 2.1 Parse `--icon-dir <path>` in `JavaScout.java` and store the value (spec: javascout-icon-config, 1 SP).
- [x] 2.2 Fall back to `Config.getIconDirectory()` when `--icon-dir` is omitted (spec: javascout-icon-config, 1 SP).
- [x] 2.3 Persist the resolved directory back to `Config` before startup (spec: javascout-icon-config, 1 SP).
- [x] 2.4 Forward the resolved icon directory to `JavaScoutApp` during construction via both static field and system property (spec: javascout-icon-config, 1 SP).

## 3. Controller wiring

- [x] 3.1 Accept icon directory parameter in `JavaScoutApp` and pass it to `MainController` (spec: javascout-icon-config, 1 SP).
- [x] 3.2 Remove the hard-coded icon path from `MainController` (spec: javascout-icon-config, 1 SP).
- [x] 3.3 Call `OSMScoutClientBuilder.withIconDirectory(directory)` when the directory is non-empty, ensuring the directory ends with a path separator (spec: javascout-icon-config, 2 SP).
- [x] 3.4 Log the configured icon directory at `MainController` initialisation for diagnostics (spec: javascout-icon-config, 1 SP).

## 4. Map rendering integration

- [x] 4.1 Verify `OSMScoutClientBuilder.withIconDirectory()` receives the configured path during native client construction (spec: map-rendering, 1 SP).
- [x] 4.2 Pass the configured icon directory to `MapParameter::SetIconPaths()` in the JNI render path so the Cairo renderer can find icons (spec: map-rendering, 3 SP).
- [x] 4.3 Confirm no icon loading is attempted when the directory string is empty (spec: map-rendering, 1 SP).
- [x] 4.4 Confirm SVG-only directory produces load errors and migrate persisted SVG default to PNG (spec: map-rendering, 1 SP).
