# Proposal

## Why

The active route is drawn with one fixed pair of colours for every presentation: an opaque red fill with a white casing. The map stylesheets otherwise distinguish a daylight and a dark presentation through the `daylight` flag, so in the dark presentation the route keeps a red that the surrounding darkened map already uses, and in the daylight presentation the white casing disappears on white residential roads. Separately, the cycle stylesheet draws its own flat, semi-transparent route line instead of the shared one, so the same route looks different depending on which map style is active.

## What Changes

- The route's paint SHALL follow the presentation the stylesheet is loaded with: a daylight and a dark variant, each defining the fill colour and the casing colour.
- The daylight route SHALL be distinguishable from the road colours it is drawn over, and its casing SHALL stay visible on the lightest road fill the style sheet uses.
- Every style sheet that can draw an active route SHALL take the route's paint from the shared route module instead of defining its own route line style, so all styles that draw a route agree; the cycle style sheet, which today uses its own single-colour rule, SHALL be changed accordingly.
- The route start/end markers, the track and the favourite/search markers that the shared module also defines stay part of one shared overlay presentation; a style sheet that includes the module therefore gets them as well.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-visualization`: the requirement that names the route's stylesheet paint changes from one fixed cased line to a presentation-dependent one, and a new requirement makes the shared module the single source of route paint for every style sheet that draws a route.

## Impact

Affected files:

- `stylesheets/include/route.oss` — the shared route, track and marker styles: presentation-dependent route fill and casing colours.
- `stylesheets/cycle.oss` — drops its own route colour and route line style and includes the shared module, like the standard and winter-sports style sheets already do.
- `Tests/src/RouteStyleColorsTest.cpp` (new) plus its entries in `Tests/CMakeLists.txt` and `Tests/meson.build` — resolves the route line styles of the real style sheets and asserts the colours and the casing/fill stacking per presentation.
- `stylesheets/standard.oss`, `stylesheets/winter-sports.oss` — consumed, not changed: they already include the shared module and declare the `daylight` flag.
- Consumers: OSMScout2, JavaScout and every other client that loads these style sheets; the active route changes colour in the daylight presentation and in the cycle style. No code, API, dependency or database change.
