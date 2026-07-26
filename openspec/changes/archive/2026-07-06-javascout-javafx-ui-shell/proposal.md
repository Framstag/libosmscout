## Why

JavaScout is currently a CLI demo that opens a database and exits. To evolve into a navigation app, it needs a UI foundation. Building more CLI features would be throwaway work — the UI is where all navigation features (map display, search, routing, GPS position) will live. Starting with an empty JavaFX window lets us add features incrementally without rewrites.

## What Changes

- Replace CLI-only JavaScout with JavaFX application
- Add main window (Stage) with menu bar, empty map panel, and status bar
- File menu: Open Database (file chooser), Exit
- Load database from CLI argument or file chooser
- Status bar shows database path and placeholder for coordinates
- Add JavaFX dependencies to Maven build
- No JNI changes — existing `OSMScoutClient` API sufficient for this step

## Capabilities

### New Capabilities
- `javafx-ui-shell`: JavaFX application window with menu bar, map panel placeholder, and status bar. Foundation for all future UI features.

### Modified Capabilities
*(none — no existing capability changes)*

## Impact

- **JavaScout/**: New JavaFX source files (`MainFrame.java`, `JavaScoutApp.java`), FXML layout, CSS styling
- **JavaScout/pom.xml**: Add JavaFX Maven dependencies (`javafx-controls`, `javafx-fxml`) and `javafx-maven-plugin`
- **JavaScout/javascout.sh**: Update launcher for JavaFX module path
- **JavaScout/README.md**: Update build/run instructions for JavaFX
- No changes to C++ code, JNI layer, or other subprojects
