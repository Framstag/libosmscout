# Design

## Context

See `proposal.md` for motivation and `specs/route-visualization/spec.md` for the contract.

Facts that shape the approach (verified on `master` @ 791d39743):

- `stylesheets/include/route.oss` is the shared overlay presentation: the `_route` casing and fill, the `_track` line, and the `_route_start` / `_route_end` / `_favorite` / `_search_selected` marker symbols and styles. Its `CONST` block declares `routeColor` and the marker colours, and the casing in the `_route` outline rule is hardcoded `#ffffff`.
- `stylesheets/standard.oss` and `stylesheets/winter-sports.oss` include it (`MODULE "include/route"`); `stylesheets/cycle.oss` does not: it declares its own `COLOR routeColor = #ae00ff88;` and its own single `[TYPE _route] WAY` rule, so the cycle style has no casing and a different colour.
- Presentation variants are already an established construct: `IF daylight { ... } ELSE { ... }` appears in the `FLAG`, `CONST` and `STYLE` sections of `standard.oss`, `cycle.oss` and `basemap-render.oss`, and the grammar also supports `ELIF`. Every style sheet that includes the route module declares `FLAG daylight = true;`.
- Flag precedence: `Parser::FLAGDEF` only calls `AddFlag` when `!config.HasFlag(name)`, so a client that sets the flag before loading (the `daylight` toggle of the navigation clients) overrides the style sheet's default, and the module's `IF daylight` is then evaluated against the client's value.
- The active route is drawn as a **poi way** of type `_route` (the JNI pushes it into `MapData::poiWays`), so its paint is a *way* line style resolved by `StyleConfig::GetWayLineStyles`. `GetRouteLineStyles` is fed by the separate `ROUTE { }` blocks that style OSM route relations and resolves nothing for `_route`.
- The order in which `GetWayLineStyles` returns the styles is **unspecified**: `PostprocessWays` groups the way line styles by slot in a `std::unordered_map` (`SortInConditionalsBySlot`) and fills the lookup tables in that container's iteration order, which differs between standard libraries and builds (observed: `[outline, fill]` with libstdc++, `[fill, outline]` with libc++). Consumers do not depend on it for the paint order - `MapPainter` gives every stroke a `wayPriority` (the style's priority) and stable-sorts the way data after preprocessing, and the main slot of a way is the one whose *slot is empty*, not the first one. The `ROUTE { }` branch in `MapPainter` does take `lineStyles.front()` as its primary style for offset decisions, which is a latent platform-dependent choice for route relations, unrelated to this change.
- `WAY#outline` sets the line style's slot to `outline`; both route styles have a display width (`2.2mm` casing, `1.5mm` fill).
- Verification available today: `OSTAndOSSTest --warning-as-error` parses each of the seven style sheets (the `CheckStyleSheet-*.oss` tests), which proves a style sheet is *valid* but says nothing about what it resolves to. `StyleConfigSymbolsTest` shows the pattern for loading a real style sheet in a test, and `Color::FromHexString` plus `Color::operator==` make resolved colours assertable. A projection can be built with `MercatorProjection::Set(coord, Magnification(magClose), width, height)`.

## Goals / Non-Goals

**Goals:**

- One place that defines the route's paint, with a daylight and a dark variant.
- Every style sheet that draws a route resolves that paint, so the route looks the same in each.
- A test that asserts the *resolved* colours and the casing/fill stacking of the real style sheets, so the values cannot drift unnoticed.

**Non-Goals:**

- No change to the route's geometry, casing widths, priorities or zoom ranges; this is about colour and about where the paint is defined.
- Not a redesign of the shared overlay module (splitting route from markers) and not a change to the non-route overlay colours.
- No renderer, painter or client code change, and no new style sheet construct.
- Not the other `naviveylin-local` leftovers (the search-scope diagnostics logging).

## Decisions

### D1: The presentation variant lives in the shared module

Chosen: the route fill and casing colours become `CONST` entries defined inside `IF daylight { ... } ELSE { ... }` in `include/route.oss`, and the casing rule uses the casing constant instead of the literal `#ffffff`.

Alternatives:
1. Keep one colour pair in the module and let each style sheet override the route colour after including it - rejected: the override would have to repeat the fill and the casing, which is exactly the duplication being removed, and the last definition would silently win.
2. Two modules, one per presentation, selected by the (conditional) `MODULE` statement in each style sheet - rejected: it forces every consumer to branch where it includes the module, duplicates the symbols and the non-route styles, and makes the shared presentation two things instead of one.
3. A separate `CONST` block per presentation in each consumer - rejected: the paint belongs to the module that owns the route and the markers; consumers would drift.

Reasoning: the flag-driven `IF`/`ELSE` in `CONST` is the construct the style sheets already use for presentation differences, and the flag precedence makes the client's daylight toggle the authority.

### D2: The daylight route is violet with an opaque, wider violet casing

Chosen: daylight fill `#ba68c8d9` (translucent violet, 85 percent alpha) with casing `#6a1b9a` (opaque
violet, 2.2mm against the fill's 1.5mm); the dark presentation keeps the current fill `#ff000088` and
casing `#ffffff`. The casing is opaque *and* wider than the fill on purpose: it covers the whole fill
footprint, so the composited centre of the route is "fill over casing" and does not take the colour of
the road underneath.

Alternatives:
1. Keep the red fill in both presentations and only change the casing - rejected: the surrounding map in the dark presentation is where the red already appears, so the two presentations would stay hard to tell apart, and the daylight white casing is the part that disappears on light road fills.
2. A dark or black daylight casing with the current red fill - rejected: a neutral casing reads as a generic outline and fights with the road borders; the violet casing keeps the casing recognisably part of the route.
3. A brighter violet or a fully saturated daylight fill - rejected: the route has to stay readable over the whole road palette; the chosen violet is in the same value range as the road fills it is drawn over, and the darker casing gives the contrast.
4. An opaque daylight fill with a dark violet casing - rejected: way labels are drawn after the way fills and carry no halo, so the black street label sits directly on the route centre. On the opaque `#7b1fa2` centre that measured 2.56:1; the translucent `#ba68c8d9` fill reads at 5.14:1 over the opaque casing, and the wider casing is what makes that value independent of the road underneath. A translucent fill over a *dark* casing would darken the centre instead and re-break the label.

Reasoning: a violet is unused by the road class fills of these style sheets, so the route cannot be confused with a road class, and a violet casing stays visible over the lightest fills (white residential roads) where the former white casing vanished. The composited centre under the label is the constraint that fixes the pair.

### D3: The cycle style sheet adopts the module and loses its own route rule

Chosen: `cycle.oss` drops its `COLOR routeColor` declaration and its `[TYPE _route] WAY` rule and includes `include/route`, like `standard.oss` and `winter-sports.oss`.

Alternatives:
1. Keep the cycle rule and only change its colour to the daylight violet - rejected: the cycle route would still lack the casing that makes the route readable over red primary roads, and the paint would still be defined twice; the inconsistency between styles is the problem, not only the hue.
2. Leave the cycle style sheet untouched and treat the shared paint as standard-only - rejected: the cycle style is a normal map style with routes, and the same route would keep looking different depending on the active style.
3. Make the cycle style sheet's own rule the shared one by moving it into the module - rejected: the module already defines the routes for the other styles; adding a second, cycle-specific route rule to it would make the module contradictory.

### D4: The cycle style sheet accepts the module's other overlay styles

Chosen: including the module brings the route start/end markers, the imported track style and the favourite/search-selection markers into the cycle style as well; that is accepted as part of the same overlay presentation.

Alternatives:
1. Split the module into a route-rules part and a marker part so a style sheet can take only the route paint - rejected for this change: it reorganises the shared overlay presentation and touches every consumer for no observable difference in the cycle style, where the markers are wanted; it is a separate change if the split is ever needed.
2. Include the module and re-hide the markers in `cycle.oss` with explicit rules - rejected: it would re-introduce per-style duplication of the overlay presentation and hide markers that the other styles show.

Reasoning: the markers belong to the route overlay (the clients register `_route_start`/`_route_end` alongside the route) and the cycle style already draws routes; the side effect is stated in the spec rather than left implicit.

### D5: A C++ test resolves the real style sheets instead of comparing images

Chosen: a new `RouteStyleColorsTest` loads `standard.oss`, `winter-sports.oss` and `cycle.oss`, resolves their `_route` line styles for both presentations and asserts the fill and casing colours, the slot of the casing and the casing/fill stacking.

Alternatives:
1. Rely on `CheckStyleSheet-*.oss` (`OSTAndOSSTest --warning-as-error`) - rejected as the only check: it proves the sheets parse, not what they resolve to, so a wrong colour would pass.
2. A rendered-pixel comparison against a reference image - rejected: the repository has no reference-image infrastructure for style sheets, and such a test is coupled to the renderer, its font stack and its anti-aliasing.
3. A test of the module alone - rejected: the module declares no `FLAG` block, so a direct load has no `daylight` value to branch on; the consumers' style sheets are the real units and each of them declares the flag.

## Flows

How the presentation reaches the resolved route paint, and what the test asserts:

```
client (OSMScout2 / JavaScout)      style sheet                     module
------------------------------      -----------                     ------
AddFlag("daylight", true/false) --> StyleConfig::flags
Load(standard.oss)              --> FLAG daylight = true   (skipped: flag exists)
                                    MODULE "include/route" -->  CONST
                                                                  IF daylight
                                                                    routeColor       = #ba68c8d9
                                                                    routeCasingColor = #6a1b9a
                                                                  ELSE
                                                                    routeColor       = #ff000088
                                                                    routeCasingColor = #ffffff
                                    STYLE
                                      [TYPE _route] WAY#outline { color: @routeCasingColor; 2.2mm; priority 99 }
                                      [TYPE _route] WAY         { color: @routeColor;        1.5mm; priority 100 }

GetWayLineStyles(buffer, projection, styles)
   -> the styles of both slots, in an unspecified order (the slots are grouped in an
      unordered_map), so the test finds the casing by its slot and asserts the
      priority relation instead of a position
   -> the painter gives every stroke its priority and stable sorts the way data, and
      the main slot of a way is the one whose slot is empty
   -> the test asserts: one style with slot "outline" (casing colour, 2.2mm, the lower
      priority) and one without a slot (fill colour, 1.5mm)
```

## Risks / Trade-offs

- [The choice of violet is a visual judgement that no test can make, only pin] → Mitigation: the road class fills of these style sheets use no violet (checked over `stylesheets/include/`; the only violet hues in the road styles are the small bus-stop marker `#bb75d9` and the bus symbol `#7885b0`, and `route_ferry` is a dashed blue line), the darker casing is the contrast carrier, and the value now lives in one constant per presentation, so it can be tuned in one place without touching consumers.
- [Including the module changes more than colours in the cycle style: the style gains route start/end markers, the track rule and the favourite/search markers] → Mitigation: the spec states it as a scenario ("Adding the shared module brings the shared overlay markers"), the design records the split-the-module alternative, and the PR calls it out; hiding them again in `cycle.oss` would duplicate the overlay presentation, which is the thing this change removes.
- [`IF daylight` in the module is evaluated against the flags of the style sheet that includes it; loading the module on its own has no declared flag] → Mitigation: all three consumers declare `FLAG daylight = true;`, the test resolves each consumer's sheet rather than the module, and the clients' flag override takes precedence by `Parser::FLAGDEF`'s `HasFlag` check.
- [The dark presentation keeps the current red, so the change is visible only in the daylight presentation and in the cycle style] → Trade-off accepted and deliberate: the red already contrasts with the darkened map, and changing it would be an unrelated restyle.
- [The style sheets are data that no renderer test covers in CI, so a future edit could change the paint and only the new test would notice] → Mitigation: the test asserts the values per presentation and per style sheet, and it is registered in both build systems, so it runs wherever the map library is built.
- [The order in which the resolved way line styles come back is unspecified, because `SortInConditionalsBySlot` groups them in a `std::unordered_map`; the first CI run of this change failed on macOS and under the memory sanitizer because the test pinned the position of the casing while the order flipped between standard libraries] → Mitigation: the test identifies the casing by its slot, asserts the priority relation that actually decides the paint order, and the failure direction was reproduced locally by iterating the slot groups in reverse. The nondeterminism is a property of `libosmscout-map` and is not changed here; the only order-sensitive consumer is the `ROUTE { }` branch of `MapPainter` (`lineStyles.front()` as its primary style), which this change does not touch.
- [Colour literals in style sheets are asserted to be lowercase by the OSS colour parser, which fails on other spellings] → Mitigation: the added literals are lowercase, and the parse is covered by the existing `CheckStyleSheet-*.oss` tests.

## Migration Plan

- Branch off `master`; style sheet and test changes only, no version bump: the database format, the type definitions and the client APIs are untouched.
- Order: shared module colours → cycle style sheet adopts the module → test plus its build entries.
- Rollback is a revert of the change; a client that pinned the old route colour by overriding the constants loses that override, which is the intended single place for the values.
