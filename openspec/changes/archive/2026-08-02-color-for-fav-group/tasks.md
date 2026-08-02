## 1. C++ Client — Star convenience methods
- [x] 1.1 Add `SetStarred()` to `FavoriteLocationService` — set/clear `attributes["starred"]`, remove key on unset
- [x] 1.2 Add `IsStarred()` to `FavoriteLocationService` — check `attributes["starred"] == "true"`
- [x] 1.3 Add unit test for star in `FavoriteLocationServiceTest`

## 2. C++ Client — Group color convenience methods
- [x] 2.1 Add `SetGroupColor()` to `FavoriteLocationService` — validate 6 hex chars, set/clear `attributes["color"]`
- [x] 2.2 Add `GetGroupColor()` to `FavoriteLocationService` — return color string or empty
- [x] 2.3 Add unit test for group color in `FavoriteLocationServiceTest`

## 3. Java JNI Bindings
- [x] 3.1 Add `setStarred`/`isStarred` native methods to `OSMScoutClient.java`
- [x] 3.2 Add `setGroupColor`/`getGroupColor` native methods to `OSMScoutClient.java`
- [x] 3.3 Implement JNI glue in `OSMScoutClient.cpp` — wire to C++ `FavoriteLocationService` methods

## 4. JavaScout — Star UI in FavLocationDialog
- [x] 4.1 Add star toggle button next to rename/delete in fav button bar
- [x] 4.2 Customize fav list cell factory to show "★" prefix for starred favorites
- [x] 4.3 Wire star toggle to `client.setStarred()` and refresh list cell

## 5. JavaScout — Star UI in FavoritePickerDialog
- [x] 5.1 Customize tree cell factory to show "★" prefix for starred favorites

## 6. JavaScout — Group color UI in FavLocationDialog
- [x] 6.1 Add color picker button per group (next to add/delete group buttons)
- [x] 6.2 Customize group list cell factory to show color swatch (`Rectangle` with fill)
- [x] 6.3 Wire color picker to `client.setGroupColor()` and refresh swatch

## 7. JavaScout — Group color UI in FavoritePickerDialog
- [x] 7.1 Customize tree cell factory to show color swatch for groups with color

## 8. Build & Verify
- [x] 8.1 Verify C++ client compiles and tests pass
- [x] 8.2 Verify Java bindings compile
- [x] 8.3 Verify JavaScout compiles and dialog renders correctly
