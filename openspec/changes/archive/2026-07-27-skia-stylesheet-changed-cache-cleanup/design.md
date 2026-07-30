## Design

### Approach

Override `StyleSheetChanged` in `MapPainterSkia` to clear both caches, matching Cairo's and Qt's approach. The implementation is trivial — Skia's `sk_sp` smart pointers auto-release when removed from the map.

### Reference Implementations

**Cairo** (`MapPainterCairo::StyleSheetChanged`):
```cpp
void MapPainterCairo::StyleSheetChanged(...)
{
  for (const auto &image : images) {
    if (image != nullptr) cairo_surface_destroy(image);
  }
  images.clear();
  for (const auto &pattern : patterns) {
    if (pattern != nullptr) cairo_pattern_destroy(pattern);
  }
  patterns.clear();
  for (const auto &image : patternImages) {
    if (image != nullptr) cairo_surface_destroy(image);
  }
  patternImages.clear();
}
```
Cairo uses raw pointers — needs manual destroy-then-clear for 3 vectors.

**Qt** (`MapPainterQt::StyleSheetChanged`):
```cpp
void MapPainterQt::StyleSheetChanged(...)
{
  patternImages.clear();
  patterns.clear();
}
```
Qt uses `QImage` and `QBrush` — `clear()` suffices.

**Skia approach**: Same as Qt — `sk_sp<SkImage>` and `sk_sp<SkShader>` are smart pointers:
```cpp
void MapPainterSkia::StyleSheetChanged(...)
{
  iconCache.clear();
  patternCache.clear();
}
```

### Cache Members to Clear

| Member        | Type                                      | Currently cleared? |
|---------------|-------------------------------------------|--------------------|
| `iconCache`   | `std::map<std::string, sk_sp<SkImage>>`   | No                 |
| `patternCache`| `std::map<std::string, sk_sp<SkShader>>`  | No                 |
| `fontCache`   | `std::map<FontDescriptor, sk_sp<SkTypeface>>` | Not listed in TODO.md, but consider |

Note: `fontCache` may also hold stale entries after a style change (font size/face could differ). Cairo clears a `fonts` map, Qt clears `fonts` too. This change scopes to `iconCache` + `patternCache` per the TODO.md gap, but `fontCache` is worth a note.

### Signature

From `MapPainter` base class:
```cpp
virtual void StyleSheetChanged(const Projection& projection,
                               const MapParameter& parameter,
                               const std::vector<MapData>& data);
```

All parameters are `[[maybe_unused]]` in Cairo and Qt — just the notification triggers clearance.

### Header Declaration

Add to `MapPainterSkia.h` in the `protected` section alongside other overrides:
```cpp
void StyleSheetChanged(const Projection& projection,
                       const MapParameter& parameter,
                       const std::vector<MapData>& data) override;
```

### TODO.md Update

Change entry from:
```
### StyleSheetChanged cache cleanup
The `StyleSheetChanged` callback is not overridden...
```
To:
```
### StyleSheetChanged cache cleanup  [DONE]
The `StyleSheetChanged` callback clears `iconCache` and `patternCache` on style change.
```
