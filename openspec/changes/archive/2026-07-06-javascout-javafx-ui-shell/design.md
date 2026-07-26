## Context

JavaScout is a Java CLI demo in `JavaScout/` that opens an OSMScout database and exits. It uses Maven and depends on `libosmscout-client-java` (JNI). The C++ client library provides database access, location search, routing, and map rendering — but JavaScout only uses `openDatabase()`.

This change replaces the CLI with a JavaFX window. The window is empty (no map rendering yet) but establishes the UI structure that all future features will plug into.

```
Current:  CLI → open DB → print → exit
Future:   JavaFX window → add features incrementally
                           (map, search, routing, GPS)
```

### Database model

Instead of a file chooser or "Open Database" menu, the app uses a **fixed database directory** stored in a config file. On startup it scans that directory and loads all `.osmscout` databases found within. This matches how mobile nav apps work (OsmAnd, Maps.me) — one maps folder, auto-discover everything.

The existing JNI already supports this: `openDatabase(path)` adds the path to `MapManager`, which triggers `LookupDatabases()` to discover all databases under that directory.

## Goals / Non-Goals

**Goals:**
- JavaFX application window with central content area and status bar
- Config file (`~/.config/javascout/config.properties`) stores database directory
- On startup: read config, scan directory, load all discovered databases
- CLI argument overrides config database directory
- Status bar shows database directory and coordinate placeholder
- Maven build produces runnable JAR with JavaFX
- Launcher script updated for JavaFX module path

**Non-Goals:**
- Map rendering (next change)
- Location search, routing, GPS (future changes)
- FXML scene builder tooling (hand-written FXML is fine)
- Multiple windows or dialogs
- Persisting window size
- Online map download (future change)

## Decisions

### JavaFX over Swing
**Decision**: JavaFX.
**Rationale**: Modern API, CSS styling, gesture events (pinch zoom, pan) for future map interaction, scene graph model maps well to Android View hierarchy for eventual port.
**Alternatives considered**: Swing — mature but no gesture support, dated look.

### Config file over file chooser
**Decision**: Properties file at `~/.config/javascout/config.properties` with key `maps.directory`.
**Rationale**: App-like experience — no file chooser dialogs. User sets it once, app remembers. CLI arg for power users. Properties format is simple, no extra dependencies.
**Alternatives considered**: `java.util.prefs` — platform-specific, no easy editing. JSON — needs a parser dependency. No config — requires CLI arg every time, not app-like.

### Database directory scanning
**Decision**: Pass the configured directory to `OSMScoutClient.openDatabase()`. The existing JNI already adds the path to `MapManager` and triggers `LookupDatabases()`, which discovers all `.osmscout` databases under that directory.
**Rationale**: Zero JNI changes needed. The C++ client library already handles multi-database discovery.
**Alternatives considered**: Scan in Java — duplicates C++ logic. New JNI method — unnecessary, existing API works.

### FXML for layout
**Decision**: Use FXML for static layout structure, programmatic code for dynamic parts (status bar updates, event wiring).
**Rationale**: Separates UI structure from logic. Easier to iterate on layout without touching Java code. Standard JavaFX pattern.
**Alternatives considered**: Pure programmatic UI — more verbose, harder to iterate.

### Controller pattern
**Decision**: Single `MainController` class injected via FXML, handles all UI events and delegates to `OSMScoutClient`.
**Rationale**: Simple enough for current scope. Can split into sub-controllers (SearchController, RouteController) as features grow.
**Alternatives considered**: MVP, MVVM — overengineered for current scope.

### Background task for database open
**Decision**: Use `javafx.concurrent.Task<Boolean>` to open database off the JavaFX Application Thread.
**Rationale**: `OSMScoutClient.openDatabase()` triggers I/O (scanning directories, loading indexes). Blocking the UI thread would freeze the window. Task provides progress/cancellation and callback on completion.
**Alternatives considered**: Open on UI thread — simple but blocks. Service with retry — overengineered.

### Maven JavaFX setup
**Decision**: Use `javafx-maven-plugin` for `mvn javafx:run`. Shade the fat JAR with `maven-shade-plugin` (already configured) including JavaFX modules.
**Rationale**: Standard JavaFX Maven workflow. Shaded JAR simplifies distribution — no separate JavaFX SDK needed at runtime.
**Alternatives considered**: jlink — produces custom runtime but more complex build. Module-path launcher — requires JavaFX SDK on target machine.

### No menu bar
**Decision**: No menu bar. The app has no File/Edit/View menus.
**Rationale**: App-like experience. Database directory is configured via config file or CLI arg, not via menu. Future features (search, settings) will get their own UI controls, not menu items.
**Alternatives considered**: Traditional menu bar — developer-tool feel, not app-like.

### JavaFX version
**Decision**: JavaFX 21 (matches Java 17+ target, latest stable LTS-aligned release).
**Rationale**: Java 17 is the minimum requirement. JavaFX 21 is the latest stable release compatible with Java 17.
**Alternatives considered**: JavaFX 17 — older, fewer fixes. JavaFX 22+ — may require newer JDK.

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    JavaScout (JavaFX)                    │
├──────────────────────────────────────────────────────────┤
│  JavaScoutApp.java          (Application entry point)    │
│  JavaScout.java             (launch logic, CLI parsing)  │
│  MainController.java        (FXML controller)            │
│  main.fxml                  (layout)                     │
│  style.css                  (styling)                    │
│  Config.java                (config file read/write)     │
├──────────────────────────────────────────────────────────┤
│  OSMScoutClient (JNI) → C++ client library              │
└──────────────────────────────────────────────────────────┘
```

### Startup flow

```
JavaScout.main(args)
  │
  ├─ CLI arg provided?
  │    ├─ yes → use as database directory
  │    └─ no  → read ~/.config/javascout/config.properties
  │              └─ key: maps.directory
  │
  ├─ Launch JavaFX (JavaScoutApp)
  │    └─ Load FXML, show stage
  │
  └─ MainController.initialize()
       └─ Task: openDatabase(directory)
            └─ OSMScoutClient.openDatabase(path)
                 └─ C++ MapManager scans directory
                      └─ loads all .osmscout databases
```

### Config file format

```properties
# JavaScout configuration
maps.directory=/home/user/maps
```

Location: `~/.config/javascout/config.properties` (Linux), `~/Library/Application Support/JavaScout/config.properties` (macOS), `%APPDATA%\JavaScout\config.properties` (Windows).

## Risks / Trade-offs

- **[Risk] JavaFX not bundled with JDK 17+** → Mitigation: Maven dependencies pull the right platform-specific binaries via `javafx-maven-plugin`. Shaded JAR includes them.
- **[Risk] JNI native library loading with JavaFX module system** → Mitigation: `--enable-native-access=ALL-UNNAMED` flag already in use. May need `--add-modules javafx.controls` in launcher.
- **[Risk] FXML becomes unwieldy as app grows** → Mitigation: Split into multiple FXML files per feature (search panel, route panel) when needed. Not a concern for this change.
- **[Trade-off] Single controller now, may need splitting later** → Accepted. Refactoring to sub-controllers is straightforward when features grow.
- **[Trade-off] JavaFX over Swing means steeper learning curve for contributors** → Accepted. JavaFX is the better long-term choice for a rich map app.
- **[Risk] Config file doesn't exist on first run** → Mitigation: App starts with empty state and shows "No maps directory configured" message. Config is created when user provides a CLI arg (saved for next run).

## Open Questions

- Should the shaded JAR include JavaFX natives for all platforms, or only Linux (dev platform)?
- What JavaFX version exactly — 21.0.2 or latest 21.x?
- Config file path: use OS-specific default (`~/.config/javascout/`) or allow override via env var?
