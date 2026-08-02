# Color and Star for Favorite Groups

## What Changes

Extend the existing favorite location system with two lightweight features:

1. **Star (favorite-of-favorites)** — each `FavLocation` gets a boolean "starred" flag. Starred favorites show a star marker in JavaScout lists.
2. **Group color** — each `FavLocationGroup` gets an RGB color stored as 6-char hex string. JavaScout shows a color swatch next to group names.

Both use the existing `attributes` map (`std::map<std::string, std::string>` in C++, `Map<String, String>` in Java) — no schema changes to the JSON persistence format beyond adding key-value pairs.

## Capabilities

### New Capabilities

- `fav-star`: Star/unstar a favorite location. Stored as `attributes["starred"] = "true"`. JavaScout shows a star icon in the fav list and picker.
- `fav-group-color`: Assign a color to a favorite group. Stored as `attributes["color"] = "RRGGBB"` (6 hex chars). JavaScout shows a color swatch in group lists and a color picker in the group dialog.

### Modified Capabilities

None. Existing `fav-location-service`, `fav-location-java-bindings`, `javascout-fav-location-ui`, and `favorite-markers` specs are extended, not changed in requirements.

## Impact

### C++ client (`libosmscout-client`)
- `FavoriteLocationService` — no structural changes. Star/color live in `attributes` map, already serialized/deserialized by existing JSON code.
- Add convenience methods: `SetStarred(group, fav, bool)`, `IsStarred(group, fav)`, `SetGroupColor(group, color)`, `GetGroupColor(group)`.

### Java bindings (`libosmscout-client-java`)
- `OSMScoutClient.java` — add `setStarred(groupName, favName, boolean)`, `isStarred(groupName, favName)`, `setGroupColor(groupName, color)`, `getGroupColor(groupName)`.
- JNI C++ glue (`OSMScoutClient.cpp`) — wire to C++ convenience methods.

### JavaScout UI
- `FavLocationDialog` — add star toggle button per favorite, color picker per group, show star marker in fav list cells, show color swatch in group list cells.
- `FavoritePickerDialog` — show star marker next to starred favorites in tree.
- `MainController` / map overlay — optionally show star marker on map for starred favorites (deferred, not in scope).

### No changes to
- JSON file format (uses existing `attributes` map)
- C++ data structs (`FavLocation`, `FavLocationGroup`)
- Import pipeline
- Other renderers or platforms
