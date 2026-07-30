## Why

The Skia map backend's `SetLineAttributes` uses a single stroke cap for both ends of a path. When `startCap` and `endCap` differ (e.g., one `Round`, one `Butt`), the less restrictive cap is applied to both ends, causing the more restrictive end to render with the wrong cap shape. Cairo and Qt backends handle this by drawing the main path with the more restrictive cap, then adding round caps at ends that need them.

## What Changes

- Modify `MapPainterSkia::DrawPath()` to handle mixed start/end caps:
  - Draw main path with the more restrictive cap (Butt > Square > Round)
  - After main stroke, draw filled circles at ends that need `Round` cap when the main stroke used a different cap
- No change to `SetLineAttributes()` signature — it still sets the more restrictive cap for the main stroke
- No change to `DrawFillStyle()` — border rendering always uses `capButt` for both ends (unchanged behavior)

## Capabilities

### New Capabilities
- `mixed-cap-style`: Render paths where start and end caps have different styles (e.g., Round start, Butt end)

### Modified Capabilities
- *(none)*

## Impact

**Affected files:**
- `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` — `DrawPath()` mixed cap handling

**Not changing:**
- `SetLineAttributes()` — signature and behavior for uniform caps unchanged
- `DrawFillStyle()` — border rendering unchanged (always Butt)
- `MapPainterSkia.h` — no new members or methods
- Other backends or core library
