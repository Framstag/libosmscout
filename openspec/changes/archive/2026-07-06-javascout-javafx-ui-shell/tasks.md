## 1. Maven Build Setup

- [x] 1.1 Add JavaFX dependencies to `pom.xml` (javafx-controls, javafx-fxml, version 21)
- [x] 1.2 Add `javafx-maven-plugin` to `pom.xml` for `mvn javafx:run`
- [x] 1.3 Configure `maven-shade-plugin` to include JavaFX modules in fat JAR
- [x] 1.4 Verify `mvn package` succeeds and produces runnable JAR

## 2. FXML Layout

- [x] 2.1 Create `src/main/resources/com/framstag/libosmscout/main.fxml` with:
  - StackPane in center (map panel placeholder)
  - StatusBar at bottom (HBox with DB path label and coord label)
  - No menu bar
- [x] 2.2 Create `src/main/resources/com/framstag/libosmscout/style.css` with basic styling

## 3. Config File Handling

- [x] 3.1 Create `Config.java` with methods:
  - `getConfigDir()` — OS-specific config directory
  - `getMapsDirectory()` — read `maps.directory` from config file
  - `setMapsDirectory(String path)` — write `maps.directory` to config file
- [x] 3.2 Handle missing config file gracefully (return null/empty)

## 4. JavaFX Application Entry Point

- [x] 4.1 Create `JavaScoutApp.java` extending `javafx.application.Application`:
  - Load FXML, set stage title "JavaScout", min size 800x600
  - Show stage
- [x] 4.2 Update `JavaScout.java` `main()` to:
  - Parse CLI arg (optional database directory)
  - If no CLI arg, read config file for `maps.directory`
  - If CLI arg provided, save to config for next run
  - Call `Application.launch(JavaScoutApp.class, args)`
  - Pass database directory to JavaScoutApp via static field or parameters

## 5. Main Controller

- [x] 5.1 Create `MainController.java` with FXML-annotated fields for map panel (StackPane), status labels
- [x] 5.2 Implement `initialize()`: if database directory provided, start background `Task` to open it
- [x] 5.3 Implement database loading in background `Task<Boolean>`:
  - Call `OSMScoutClient.openDatabase(directory)`
  - On success: update status bar with directory path
  - On failure: show "No databases found" in status bar
- [x] 5.4 Wire status bar updates: database directory on left, coordinate placeholder on right

## 6. Launcher & Docs

- [x] 6.1 Update `javascout.sh` for JavaFX module path and fat JAR
- [x] 6.2 Update `README.md` with JavaFX build/run instructions and config file docs
- [x] 6.3 Verify end-to-end: build, launch with CLI arg, see status bar update, launch without arg, see empty state
