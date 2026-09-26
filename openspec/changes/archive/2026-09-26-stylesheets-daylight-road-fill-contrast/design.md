# Design

## Context

See `proposal.md` for motivation and `specs/road-fill-contrast/spec.md` for the contract.

Facts that shape the approach (verified on the `stylesheets-route-presentation-colors` state of `master`):

- `stylesheets/include/roads.oss` draws the road classes and consumes per-style-sheet constants: `[TYPE highway_motorway]` uses `@motorwayColor`, `darken(@motorwayColor,0.4)` for its outline and `@thinMotorwayColor` below the full road width, and the trunk, primary and secondary classes follow the same pattern. The fills themselves are declared in `stylesheets/standard.oss` and `stylesheets/winter-sports.oss` (`IF daylight` / `ELSE`), which both include the module.
- Which of the two motorway rules applies is decided by a SIZE filter: `[SIZE 20m 0.45mm:3px<]` selects the cased paint (outline plus fill) once 20 m of road are at least 0.45 mm or 3 px on screen, `[SIZE 20m <0.45mm:3px]` selects the thin paint below that. `SizeCondition::Evaluate` compares `Projection::GetMeterInMM()` / `GetMeterInPixel()`, and a projection built without an explicit dpi has `dpi=0.0`: the derived values are NaN, both comparisons fall through to the pixel branch, and only the thin paint resolves. A test that wants the cased paint has to pass a dpi.
- The fills are `COLOR` literals in the `CONST` block, but three of the constants a style sheet derives from them are computed: `thin*Color = lighten(@*Color, 0.2/0.3)`, the shield backgrounds (`@*ShieldColor = darken(@*Color, 0.45)`), and `motorwayJunctionLabelColor = lighten(@motorwayColor, 0.3/0.5)`.
- The shields paint **white** text: `[TYPE highway_motorway] WAY.SHIELD { label: Ref.name; color: #ffffff; backgroundColor: @motorwayShieldColor; borderColor: #ffffff; ... }` in `stylesheets/include/roads.oss`, so their background is the contrast carrier, and a shield style is resolvable in a test through `StyleConfig::GetWayPathShieldStyle(buffer, projection)`.
- The way label is black in the daylight presentation (`COLOR wayLabelColor = #000000;` in `standard.oss`) and is drawn on the way fill with no halo of its own, so the fill decides the label's contrast.
- `Color` stores its channels as `double` in `[0,1]`: a colour parsed from a literal is `byte/255.0`, while `lighten()` recomputes the channel and lands within one ULP of it. `Color::operator==` compares those doubles exactly and `ToHexString()` rounds them, so a test that pins a *derived* colour has to compare hex strings (or accept a tolerance); the sibling `RouteStyleColorsTest` compares `Color` objects because the route colours are literals.
- Verification available today: `OSTAndOSSTest --warning-as-error` parses each style sheet (`CheckStyleSheet-*.oss`), which proves validity but says nothing about what a rule resolves to; `StyleConfig::GetWayLineStyles` resolves the paint of a real style sheet for a `FeatureValueBuffer` and a `Projection`.

Measured on the daylight literals (sRGB relative luminance, the label black and the shield text white):

| fill | before | black label | after | black label |
|------|--------|-------------|-------|-------------|
| motorway | `#4440ec` | 3.19:1 | `#7d7af5` | 5.97:1 |
| trunk | `#7674ec` | 5.48:1 | `#a3a1f5` | 8.97:1 |
| primary | `#ec4044` | 5.38:1 | `#f58b8b` | 8.95:1 |
| secondary | `#fdac44` | 11.16:1 | `#fdd08a` | 14.57:1 |

White shield text on the new fills would have read 3.52:1 / 2.34:1 / 2.35:1 (motorway / trunk / primary), below the 4.5:1 a small label needs; on the darkened shield backgrounds it reads 8.78:1 / 6.64:1 / 6.66:1.

## Goals / Non-Goals

**Goals:**

- The daylight fills of the four road classes carry the black way label at the contrast a label needs, in both style sheets that declare their own fills.
- Every constant a style sheet derives from a road fill is derived from the *new* fill, so the class stays recognisable as one class: a thin fill that is a lighter variant of its fill but not washed out on the land, a shield background that stays dark enough for its white text, and a junction label that stays visible on the road.
- The dark presentation keeps the fills it has.
- A test asserts the *resolved* fills and the resolved shield backgrounds of the real style sheets, so the values cannot drift unnoticed.

**Non-Goals:**

- No change to the road geometry, widths, zoom ranges or the SIZE filters; this is about the daylight colours and the constants derived from them.
- No change to the other road-bearing style sheets: `cycle.oss`, `motorways.oss`, `railways.oss`, `public-transport.oss` and `boundaries.oss` keep their own palettes, and the cycle style sheet's route-independent fills are not part of this change.
- No change to the dark presentation, no new style sheet construct, no renderer, painter, client or type-definition change.
- No attempt to make the map style accessible in general (pattern colours, icons, other overlays); this change is about the road fills and the label and shield text drawn on them.

## Decisions

### D1: Lighten the fills instead of changing the label or giving it a halo

Chosen: the four daylight fills move up in lightness (`#4440ec` → `#7d7af5`, `#7674ec` → `#a3a1f5`, `#ec4044` → `#f58b8b`, `#fdac44` → `#fdd08a`), which carries the black way label from 3.19:1 to 5.97:1 on the worst class.

Alternatives:

1. Draw a halo (an outline in the land colour) behind the way label - rejected: the label is drawn by the way label provider after the way fills and has no halo construct in the style sheets, so this would add a rendering feature for a palette problem, and it would change the look of every label on every map, not only the road classes.
2. Keep the fills and make the label white on the dark classes - rejected: the label colour is one constant per presentation for all way labels, so a per-class label colour would either split that constant or turn the label white on the light classes too, where it loses contrast.
3. Lighten only the motorway (the one class below 4.5:1) - rejected: the classes would then be ordered by lightness in a way that no longer matches their importance (the motorway would be lighter than the secondary class or close to it), and the neighbouring classes share the same visual weight in this palette.

Reasoning: the label contrast is a property of the surface the label is drawn on, the surface is the class's fill, and the palette already distinguishes the classes by lightness - so the fill is the one place where the fix belongs and where the class ordering can be preserved.

### D2: Keep the road classes ordered by lightness and out of each other's way

Chosen: the four fills keep their hue and their order (blue motorway, lighter blue trunk, red primary, orange secondary), and the order survives the move: the luminance step between the motorway and the trunk narrows from 1.72:1 to 1.50:1, the trunk and the primary class stay at the same luminance, and the primary class stays clearly darker than the secondary class (2.07:1 → 1.63:1).

Alternatives:

1. Move the classes into one hue ramp - rejected: the hues are the established identity of the classes in these style sheets and in the clients that ship them, and a ramp would make the primary and secondary class look like the motorway.
2. Reuse the route's violet for a road class - rejected: the route paint is deliberately the only violet on the map (`stylesheets-route-presentation-colors`), and a violet road class would collide with it.
3. Take the land colour (`#f1eee9`) as the lightest step - rejected: a road class that reads as land stops being a road class, and the outline of the cased paint is what separates the two.

Reasoning: the daylight palette is read as a whole; keeping the hues and the order means the change is a legibility fix that no client has to re-learn, while the derived constants below are what makes the classes stay distinguishable from the land.

### D3: Derive the thin fills with 0.2 instead of 0.3 and the shields with a darkened constant

Chosen: `thin*Color = lighten(@*Color, 0.2)` (was 0.3) and `@*ShieldColor = darken(@*Color, 0.45)` (was the fill itself), plus `motorwayJunctionLabelColor = lighten(@motorwayColor, 0.3)` (was 0.5).

Alternatives:

1. Leave the derived constants as they are - rejected: the thin variant is lightened *from* the fill, so it moves toward the land colour as the fill lightens (the new motorway fill is already 3.04:1 against the land, its 0.2 variant 2.28:1, and its 0.3 variant 1.99:1), and white shield text on a light fill is exactly the contrast failure this change removes (3.52:1 / 2.34:1 / 2.35:1).
2. Give the thin fills and the shields their own literal colours - rejected: they are variants of one class, and a literal would have to be edited again the next time the fill moves; the derivation is what keeps them consistent.
3. Darken the thin fill instead of lightening it less - rejected: the thin fill is drawn where the road is too small to be cased, so it has to stay *lighter* than the full width fill of the same class to keep the visual hierarchy, which is what the reduced factor does.
4. Keep the junction label at `lighten(fill, 0.5)` - rejected: at 0.5 the label colour on the new, lighter fill loses the step it needs to separate from the road it is drawn on; 0.3 keeps a visible step and stays near the road's own value range.

Reasoning: these three constants exist *because* of the fill, so the fill's move has to carry them; deriving them (rather than pinning literals) means the next palette change moves them along.

### D4: A C++ test pins the resolved fills and shield backgrounds

Chosen: `Tests/src/RoadStyleColorsTest.cpp` loads `map.ost` plus `standard.oss` and `winter-sports.oss`, resolves the line styles of the four classes with `StyleConfig::GetWayLineStyles` and the shield styles with `GetWayPathShieldStyle`, and asserts the daylight fills, their thin variants, the stacking of the cased paint and the shield backgrounds; a dark section asserts that the dark fills differ from the daylight ones.

Alternatives:

1. Compare rendered images (a golden-image test) - rejected: the repository has no golden-image infrastructure, the result depends on fonts, backends and antialiasing, and the assertion wanted here is a colour, not a picture.
2. Extend `StyleConfigSymbolsTest` - rejected: that test answers a different question (which types resolve which symbols); the road paint deserves its own, named test like the sibling `RouteStyleColorsTest`.
3. Assert the constants by reading the style sheet files as text - rejected: that pins the syntax, not what a client resolves, and it would pass if a rule stopped matching its type.

Reasoning: the style sheets are data that parses cleanly while resolving to a wrong colour, so the pin has to go through the resolver the renderers use. Because the cased and the thin paint are selected by zoom, the test tries a list of zooms (the widths differ per style sheet) and a projection with an explicit dpi instead of a fixed magnification.

### D5: The fills are compared as hex strings, not as `Color` objects

Chosen: the test compares `Color::ToHexString()` of the resolved style against the expected literal.

Alternatives:

1. Compare `Color` objects - rejected: `lighten()` lands one ULP away from `byte/255.0`, so exact equality is a coin flip on the last bit and depends on the standard library's floating point and the build flags; the sibling route test can compare `Color` only because every route colour is a literal.
2. Compare with an epsilon per channel - rejected: it hides the difference the test is about (a palette move of one step) and reads worse than the byte a style sheet author writes.

Reasoning: the unit a palette decision is made in is the byte triple, so the test asserts that unit; `ToHexString()` is the same conversion the style sheet parser and the debug output use.

## Flows

How a fill reaches the label and the shield, and what the test asserts:

```
                   standard.oss / winter-sports.oss            include/roads.oss
                   --------------------------------            -----------------
IF daylight {
  motorwayColor  = <literal>            --+
  trunkColor     = <literal>              |  @motorwayColor / @trunkColor / ...
  primaryColor   = <literal>              |  @thin*Color, @*ShieldColor
  secondaryColor = <literal>            --+
} ELSE { darken(<literal>, 0.3) }         |
thin*Color       = lighten(@*Color, 0.2)  |
 *ShieldColor    = darken(@*Color, 0.45)  |
                                          v
                                   [TYPE highway_motorway]
                                   [SIZE 20m 0.45mm:3px<] -> WAY#outline + WAY  (cased)
                                   [SIZE 20m <0.45mm:3px] -> WAY @thinMotorwayColor
                                   [TYPE highway_motorway] WAY.SHIELD { color: #ffffff; backgroundColor: @motorwayShieldColor }

painter: way fills -> way label (black, no halo) -> shield background with white shield text

test: LoadStyleSheet(map.ost + sheet, daylight) -> MercatorProjection(coord, zoom, dpi=96, 300x400)
      GetWayLineStyles(buffer(highway_motorway|trunk|primary|secondary), projection, styles)
        -> the fill is the style whose slot is empty, the casing the one with slot "outline"
        -> the cased paint is the first zoom of the list where both slots resolve, the thin paint the
           first zoom where only the fill resolves
      GetWayPathShieldStyle(buffer(highway_motorway), projection) -> GetBgColor() / GetTextColor()
        -> the test asserts: the fill per class per sheet, the thin variant, the outline darker than
           the fill and wider than it, the shield background and its white text
```

## Risks / Trade-offs

- [The lightness of the palette is a visual judgement that no test can make, only pin] → Mitigation: the four fills are pinned per style sheet, the worst of them is measured (3.19:1 → 5.97:1 for the black label), and the values live in one `CONST` block per style sheet, so a later tuned value is one edit.
- [The change is invisible to a client that overrides the road colours itself] → Trade-off accepted: a client that pins its own palette keeps it; the constants stay the single place the style sheet offers.
- [The lighter fills change the look of the whole daylight map, not only the labels] → Mitigation: the hues, the order and the widths are unchanged, the dark presentation is untouched, and the shield and thin constants follow so that the classes stay distinguishable from the land and from each other.
- [The road fill colours are also consumed by the other style sheets that include `roads.oss` (if a consumer is added later, it inherits whatever its own constants are)] → Mitigation: the module consumes the constants per style sheet, so a new consumer declares its own fills; this change does not move the constants into the module.
- [The shields gain a darker background, which is a second visible change] → Deliberate: white shield text on the new fills would read 3.52:1 / 2.34:1 / 2.35:1, below the label contrast, and on the old trunk and primary fills it was already 3.83:1 / 3.90:1.
- [The test resolves styles through `GetWayLineStyles`, whose return order is unspecified (the slots are grouped in an `unordered_map`)] → Mitigation: the test identifies the strokes by their slot, never by position, exactly like the sibling route test.
- [A test that pins colours fails when the palette moves on purpose] → Trade-off accepted and intended: the pinned values are the contract of this change; the next palette change updates the test as part of the change.

## Migration Plan

- Branch off `master`; style sheet and test changes only, no version bump: the database format, the type definitions and the client APIs are untouched.
- Order: the four daylight fills and the derived constants in both style sheets → the test plus its entries in both build systems.
- Rollback is a revert of the change; a client that pinned the old road colours is unaffected because the style sheets keep their own constants.
