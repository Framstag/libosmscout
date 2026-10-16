# Design

## Context

See `proposal.md` — Why. The relevant current state on `master`:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the single JNI translation unit. Every render entry
  point it has (`render`, `renderWithRouteAndPois`, `renderInto`) needs a caller-supplied frame: a viewport,
  a center, a magnification, a dpi and a rotation. A search-result or POI row has none of those; it knows the
  type name of its entry and the pixel size of its leading icon.
- `ClientData` (`OSMScoutClient.cpp`) already carries what the entry point needs: the `DBThread` (the loaded
  databases, their style configurations, and the configured icon directory) and `preferSymbolIcons`, set from
  Java by `setPreferSymbolIcons(boolean)`.
- `osmscout::StyleConfig` resolves a style entry for a type through its own selectors:
  `GetNodeIconStyle(FeatureValueBuffer, projection)` and `GetAreaIconStyle(TypeInfoRef, FeatureValueBuffer,
  projection)`. `IconStyle` then offers `GetSymbol()` (a vector `Symbol`) and `GetIconName()` (a PNG name),
  and `IsVisible()` is false when it has neither.
- `MapPainter::LayoutPointLabels` (`libosmscout-map/src/osmscoutmap/MapPainter.cpp`) draws the raster icon
  first and falls back to the vector symbol, and draws the symbol first when
  `MapParameter::GetPreferSymbolIcons()` is on and the entry has one.
- `osmscoutmapcairo::LoadPNG` and `osmscoutmapcairo::SymbolRendererCairo` are the Cairo backend's own
  loader and symbol painter; both are already linked into the bridge (`MapPainterCairo.h` is included under
  the same `HAVE_MAP_CAIRO` guard).
- The reference implementation is the NaviVeylin branch's `renderTypeIcon` (`origin/naviveylin-local`,
  commits `b199bdf8f` and `f3d7e046e`). It is one self-contained function plus two static drawing helpers
  and their constants; `master` carries newer work in the same file that must stay untouched.

## Goals / Non-Goals

**Goals:**

- One entry point that returns a type's stylesheet artwork into a square, with no map frame, viewport or
  projection of the caller's.
- The artwork is what the active stylesheet declares for that type, and the client's icon-versus-symbol
  preference applies as it does on a map.
- An unusable request returns `null` and never faults; a failure a caller cannot see is logged with the stage
  that decided it.
- The two decisions the entry point makes in arithmetic — the fit scale and the conversion of a surface pixel
  into the word Java receives — are testable on the host, without a database, a style or a Cairo surface.
- Take over the reference implementation's behaviour without taking over the unrelated work that sits in the
  same file on the reference branch.

**Non-Goals:**

- Changing what a stylesheet declares, or the icon directory the client is configured with.
- Changing any existing render entry point, or adding a Java-side `render` change.
- Rendering a type's artwork from a loaded object (a rule that selects on a tag still has no object to match;
  see D1).
- A new database, map, style or file format, so no `FileFormatVersion.md` version bump applies.

## Decisions

**D1 — The artwork comes from the stylesheet, resolved through a tag-less feature buffer.**
The entry point looks the type up in the database's `TypeConfig` and resolves the icon style from the
database's `StyleConfig` with a `FeatureValueBuffer` that carries the type and no tag values.
Alternatives:
- *Load the object first and resolve from real tags*: a search row has a type name and a coordinate at best,
  not the tags of the object the stylesheet's rule selected on, and loading an object per icon is the work
  this entry point exists to avoid.
- *Carry the tags the caller knows*: it would make the entry point's result depend on which tags a caller
  happened to have, so two rows of one type could draw differently.
Chosen because a tag-selecting rule then matches nothing and yields no artwork, which the spec delta states
as the wanted outcome (an absent icon, not an invented one).

**D2 — The projection is the entry point's own, at a fixed detail magnification.**
A `MercatorProjection` is created inside the entry point, centered at the null island, with a zero rotation,
`NAVIVEYLIN_TYPE_ICON_DPI` (96.0) as its reference resolution and the requested square as its viewport, and
with the magnification *level* `osmscout::Magnification::magVeryClose`.
Alternatives:
- *The factor constructor, `Magnification(16.0)`*: it reads its argument as a magnification factor, i.e.
  level 4, at which a stylesheet defines no POI icon at all — the reference implementation's first version
  did this and every icon came back empty (`f3d7e046e`). The level constructor is the one that names a detail
  level, which is what a stylesheet's icon rules are written against.
- *Ask the caller for a magnification*: a row that wants a leading icon has no map state to pass, and the
  square's size already carries the only resolution the caller knows about.
- *Reuse a projection from a previous frame*: makes one row's artwork depend on another row's last render.
Chosen because the artwork a stylesheet declares for a POI does not vary with the viewport it is drawn in,
and a fixed detail level is the one at which those icons are declared.

**D3 — The preference prefers the symbol; without it the map's own order decides.**
With `setPreferSymbolIcons(true)` and an entry that has a symbol, the symbol is drawn. Otherwise the raster
icon is tried first and the symbol is the fallback.
Alternatives:
- *Gate the symbol on the preference* (the reference implementation's first version): with the preference
  off, a symbol-only entry — the normal case for most stylesheet icons, which ship as vector symbols without
  a PNG — drew nothing, and a row lost its icon. `f3d7e046e` corrected it.
- *The symbol always, when present*: contradicts the client-wide preference the map render already honours.
Chosen because it makes the entry point agree with `MapPainter::LayoutPointLabels`: "prefer" chooses between
two available renderings, it does not disable one of them.

**D4 — The two pure decisions live in a dependency-free header, and the fit fraction has one spelling.**
`libosmscout-client-java/src/type_icon_pixels.h` (`namespace naviveylin`) holds `TypeIconFitFraction`,
`TypeIconFitScale(side, extent)` and `TypeIconArgbWord(b, g, r, a)`, with no JNI and no libosmscout include,
following `frame_pixel_layout.h` and `route_length.h`. The entry point keeps no copy of the fit fraction:
both drawing helpers scale through `TypeIconFitScale`.
Alternatives:
- *Keep the arithmetic inline in the JNI function* (the reference implementation's shape): the only way to
  cover it would be a rendering test, and no map database of the format the library expects exists in this
  tree, let alone one whose stylesheet declares a known icon for a known type.
- *Keep a `NAVIVEYLIN_TYPE_ICON_FIT` define in the JNI file as well*: two spellings of one number, which can
  drift, and the entry point's own copy would then be the one nothing tests.
Chosen because the two decisions are arithmetic whose mistakes are invisible in a picture — a wrong fit
draws a small or clipped icon, a missing un-premultiplication draws a black fringe on every anti-aliased
edge — and a host Catch2 test is the only cheap way to pin them. The other three constants stay `#define`s in
the JNI file (`NAVIVEYLIN_TYPE_ICON_MAGNIFICATION_LEVEL`, `NAVIVEYLIN_TYPE_ICON_DPI`,
`NAVIVEYLIN_TYPE_ICON_MAX_SIZE`), where they are used.

**D5 — The result is straight ARGB, built from the surface's own byte order.**
`CAIRO_FORMAT_ARGB32` stores premultiplied channels in native byte order (B,G,R,A on the platforms this
bridge builds for). `TypeIconArgbWord` divides the alpha back out for a pixel whose alpha is neither 0 nor
255 and packs `0xAARRGGBB`, which is what the allocating render entry point hands Java and what Android's
non-premultiplied `ARGB_8888` expects. A pixel with alpha 0 becomes `0x00000000`.
Alternatives:
- *Hand the premultiplied word over*: Android's `ARGB_8888` is not premultiplied, so every partially
  transparent pixel would arrive darkened by its own alpha — a black fringe around each anti-aliased edge.
- *Pre-multiply into the buffer instead and let the caller un-premultiply*: moves a per-pixel cost and a
  format detail onto every caller, and disagrees with the entry point the new one is documented against.
Chosen because the layout must be the allocating entry point's, so a caller can hand the same `int[]` to
`Bitmap.setPixels` either way.

**D6 — A request the entry point cannot satisfy returns `null` and says so; a type without artwork is silent.**
`sizePx` outside 1..512, a `null` name, no client data or no `DBThread`, and a bridge built without Cairo all
return `null` without faulting. Inside the job, each stage that decides an absence logs
`[JNI] renderTypeIcon stage=<stage> type=<name> px=<size>`, and the stage that is simply "the stylesheet
gives this type no icon" (`!iconStyle || !iconStyle->IsVisible()`) does not log at all, because the caller
reports that absence itself.
Alternatives:
- *Report the absence only to the caller*: a missing icon that the stylesheet does not explain is then not
  diagnosable at all.
- *Log every absence, including the normal one*: a search list of streets and addresses would fill the log
  with entries that are not defects.
Chosen because the two cases differ: one is a defect or a misconfiguration, the other is the answer.
*Deviation from the reference implementation, disclosed in the PR body:* the spec delta requires every
stage warning to carry the requested size, and the reference logs `px=` only in the stages after the
projection exists. The three earlier warnings (`no-style-config`, `no-type-config`, `unknown-type`) therefore
carry it here as well; the size is a parameter of the request and is known at every stage. Nothing else about
those warnings changes.

## Data flow

```
Java: renderTypeIcon("amenity_parking", 48)
  |
  v
guard: client data? DBThread? name != null? 0 < sizePx <= 512?      -> null, no log
  |
  v
RunSynchronousJob(candidates = loaded databases + basemap)
  |
  for each db: StyleConfig? TypeConfig? TypeInfo(name)?             -> null + stage warning
  |
  FeatureValueBuffer{type, no tags}
  MercatorProjection (own, magVeryClose, 96 dpi, side x side)
  |
  IconStyle = GetNodeIconStyle(buffer) else GetAreaIconStyle(type, buffer)
  |   !visible -> continue (silent: the stylesheet gives no icon)
  |
  cairo_image_surface ARGB32(side, side)
  |
  preferSymbolIcons && symbol ?  symbol first
                              :  PNG icon first, symbol when the image is unusable
  |   neither -> null + stage warning
  |
  per pixel: cairoData[b,g,r,a] -> TypeIconArgbWord -> 0xAARRGGBB     (helper, host-tested)
  |
  v
NewIntArray(side * side) + SetIntArrayRegion -> int[]  (or null when nothing rendered)
```

## Risks / Trade-offs

- *The entry point cannot be exercised end to end here*: `maps/` holds format v26 databases while the library
  expects v27, so no database with a usable stylesheet can be opened on this machine; the entry point is
  verified by building it, by the host test of its two decisions, and by inspection of its stage contract
  (recorded as NOT RUN in `verification.md`).
- *A stylesheet icon that ships as SVG only cannot be drawn*: `LoadPNG` reads PNG, as `MapPainterCairo` does,
  so such an entry falls back to its symbol — which is exactly why D3 matters.
- *The preference and the image order are decided here, not by `MapPainter`*: the two implement the same
  rule, but nothing links them mechanically. `Tests/src/MapPainterIconSymbolPreferenceTest.cpp` pins the
  painter's half; the entry point's half is a three-line branch.
- *A tag-selecting rule yields no artwork*: stated in the spec delta and in the Javadoc; a consumer that wants
  the brand- or religion-specific icon has to render a map frame or load the object.
- *One more native declaration without a Java-side caller in this repository*: no code here calls it (the
  Android app does), so a regression in the entry point would not show up in this repository's own tests.
- *`NAVIVEYLIN_TYPE_ICON_DPI` is a reference resolution, not a measurement*: it only fixes the projection's
  scale for resolving a symbol, and the fit scale overrides the artwork's final size either way.

## Migration Plan

The reference implementation's material is taken over file by file: the three includes, the constants, the
two drawing helpers, the entry point and the Java declaration, with the pixel conversion replaced by the
helper of D4. `master`'s existing content in `OSMScoutClient.cpp` is untouched outside the inserted block
(the diff is insertions only). The Java declaration is part of this change, so the JNI function is not dead
bridge code (`scripts/check-jni-signatures.sh` counts one native declaration more and reports the entry point
as declared).

Rollback is a revert of the commits: the entry point disappears, the file returns to its previous content, the
helper and its test are removed and the two test registrations are dropped. Nothing persisted (a database, a
file format, a settings value) is affected.

## Open Questions

- Whether a consumer should be able to pass the tags it knows, so a tag-selecting rule can match: deferrable,
  no caller asked for it, and D1 argues against it.
- Whether the fit fraction (0.9) and the maximum square (512) should be configurable per call: deferrable, and
  the default is the caller's own layout decision, which it can make by cropping the returned square.
- Whether the entry point should also serve a whole list of types in one job: the per-call `RunSynchronousJob`
  is one database read lock per icon; a caller drawing a long list may want a batch entry point. Deferrable,
  and it is a different signature.
