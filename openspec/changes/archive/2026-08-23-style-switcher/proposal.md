# Style Switcher

## Why

JavaScout renders the map with a single stylesheet fixed at startup. libosmscout ships multiple stylesheets (`standard`, `cycle`, `railways`, `winter-sports`, …) that change the map's appearance for different use cases. Users of JavaScout currently have no way to pick another style without rebuilding or editing configuration, and the Java client layer exposes no runtime style-switching API at all — even though the underlying client libraries already support loading a different stylesheet at runtime.

## What Changes

- Add a "Switch Style…" menu item to the JavaScout main menu that opens a chooser dialog.
- The chooser lists all available styles as a dropdown (combobox).
- Style candidates are the top-level `*.oss` files in the stylesheets directory; the style name is derived from the file name (e.g. `standard.oss` → "standard").
- Selecting a style immediately redraws the map with the new style; the map view (position, zoom) is preserved.
- Style enumeration and switching logic is implemented in the client and client-java libraries where feasible, so the capability is reusable outside JavaScout.

## Capabilities

### New Capabilities

- `javascout-style-switcher`: JavaScout UI for switching map styles — main menu entry, selection dialog with a combobox listing available styles, and immediate map redraw on selection.
- `client-java-style-switching`: Client library support for enumerating available stylesheets and switching the active stylesheet at runtime, including notification that a redraw is needed.

### Modified Capabilities

None.

## Impact

- `libosmscout-client` (C++): runtime stylesheet loading already exists; switching support may reuse or extend it. No new dependencies.
- `libosmscout-client-java`: Java client API (`OSMScoutClient`, builder) extended with style enumeration and style switching; native implementation in `libosmscout-client-java/src`.
- `JavaScout`: `MainController` (menu and dialog wiring, redraw trigger), renderer invocation, configuration of the stylesheet directory.
- `stylesheets/`: source directory scanned for top-level `*.oss` files; not modified.
- Build systems: CMake and Meson for the affected libraries.
