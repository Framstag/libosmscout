## Context

The favorite location system uses an extensible `attributes` map (`std::map<std::string, std::string>` in C++, `Map<String, String>` in Java) on both `FavLocation` and `FavLocationGroup`. JSON serialization already handles arbitrary key-value pairs in this map — no format changes needed.

Current JavaScout UI (`FavLocationDialog`, `FavoritePickerDialog`) renders group and favorite names as plain text in `ListView`/`TreeView` cells.

## Goals / Non-Goals

**Goals:**
- Store star state and group color using existing `attributes` map — zero schema migration
- Add convenience C++ methods on `FavoriteLocationService` for star/color access
- Add Java JNI bindings and `OSMScoutClient` methods
- Add JavaScout UI: star toggle, color picker, visual indicators in lists

**Non-Goals:**
- No map overlay rendering of star/color (deferred)
- No changes to other platforms (Qt, iOS, Android)
- No changes to JSON file format or data structs
- No multi-color or gradient support — simple 6-char RGB only

## Decisions

### Store star/color in `attributes` map (not new fields)

**Decision:** Use existing `attributes` map with well-known keys `"starred"` and `"color"`.

**Rationale:** Zero schema migration. JSON serialization already handles it. No struct changes needed. The `attributes` map was designed for exactly this kind of extension.

**Alternatives considered:**
- New bool/string fields on structs — would require JSON serializer changes and break backward compat
- Separate star/color tables — overengineered for simple flags

### Convenience methods on `FavoriteLocationService` (not raw attribute access)

**Decision:** Add `SetStarred`/`IsStarred`/`SetGroupColor`/`GetGroupColor` methods.

**Rationale:** Encapsulates key name convention, validation (color must be 6 hex chars), and consistent attribute key removal on unset. Callers don't touch the map directly.

### JavaScout: cell factory customization (not custom controls)

**Decision:** Use `ListCell`/`TreeCell` customization with `setGraphic()` for star/color indicators.

**Rationale:** Matches existing pattern in both dialogs. No new FXML or custom component classes needed.

### Color picker: JavaFX `ColorPicker` control

**Decision:** Use standard JavaFX `ColorPicker` for group color selection.

**Rationale:** Built-in, well-supported, provides color swatch grid and custom RGB input. Convert selected `Color` to 6-char hex string for storage.

## Risks / Trade-offs

- **Color validation in C++ only** — Java side trusts the C++ layer. If someone edits JSON by hand, invalid colors are silently ignored.
- **Star as string "true"** — not a native bool in JSON. The `attributes` map is string→string, so "true"/absent is the natural encoding. Slightly more parsing overhead but negligible.
- **No star on map** — user requested star marker in list only. Map rendering of stars is a future enhancement.
