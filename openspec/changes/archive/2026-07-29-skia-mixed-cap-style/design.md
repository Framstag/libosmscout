# Mixed Cap Style Handling — Design

## Overview

Add mixed cap style support to `libosmscout-map-skia`'s `DrawPath()`. When `startCap` and `endCap` differ, the path is drawn with the more restrictive cap, and round caps are added at ends that need them.

## Cap Restrictiveness Order

From most restrictive to least:
1. `capButt` — flat end, no extension
2. `capSquare` — flat end, extends half line width
3. `capRound` — rounded end, extends half line width

The main path is always drawn with the most restrictive of the two caps.

## Drawing Pipeline

```
DrawPath(color, width, dash, startCap, endCap, coordRange)
    │
    ├── Determine effective cap = moreRestrictive(startCap, endCap)
    │
    ├── Draw main path with effective cap
    │   └── SetLineAttributes(paint, color, width, dash, effectiveCap, effectiveCap)
    │   └── draw->drawPath(path, paint)
    │
    ├── Draw start cap if needed
    │   └── if startCap == Round && effectiveCap != Round
    │       └── draw->drawCircle(startX, startY, width/2, fillPaint)
    │
    └── Draw end cap if needed
        └── if endCap == Round && effectiveCap != Round
            └── draw->drawCircle(endX, endY, width/2, fillPaint)
```

## Cap Resolution

```cpp
static SkPaint::Cap ResolveEffectiveCap(LineStyle::CapStyle startCap,
                                          LineStyle::CapStyle endCap)
{
    // Use the more restrictive cap (Butt > Square > Round)
    if (startCap == LineStyle::capButt || endCap == LineStyle::capButt) {
        return SkPaint::kButt_Cap;
    }
    if (startCap == LineStyle::capSquare || endCap == LineStyle::capSquare) {
        return SkPaint::kSquare_Cap;
    }
    return SkPaint::kRound_Cap;
}
```

## Round Cap Drawing

Round caps are drawn as filled circles at the path endpoint:

```cpp
SkPaint capPaint;
capPaint.setAntiAlias(true);
capPaint.setStyle(SkPaint::kFill_Style);
capPaint.setColor(color);
draw->drawCircle(x, y, width / 2.0, capPaint);
```

This matches the visual result of a zero-length line with Round cap (diameter = lineWidth), but is simpler and more direct.

## Edge Cases

| Case | Behavior |
|------|----------|
| Both caps Round | Main path uses Round cap, no extra circles needed |
| Both caps Butt | Main path uses Butt cap, no extra circles needed |
| Both caps Square | Main path uses Square cap, no extra circles needed |
| startCap=Round, endCap=Butt | Main path uses Butt, circle at start |
| startCap=Butt, endCap=Round | Main path uses Butt, circle at end |
| startCap=Round, endCap=Square | Main path uses Square, circle at start |
| Dashed path + mixed caps | Main dash with restrictive cap, circles at ends |
| Very narrow path (width < 1px) | Circle radius < 0.5px, effectively invisible — acceptable |

## Not Changing

- `SetLineAttributes()` — still sets uniform cap for main stroke
- `DrawFillStyle()` — border rendering always uses Butt for both ends
- `MapPainterSkia.h` — no new members or methods
