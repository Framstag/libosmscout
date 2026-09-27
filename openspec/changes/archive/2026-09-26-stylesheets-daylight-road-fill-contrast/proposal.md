# Proposal

## Why

The way label of a road is drawn directly on the road's fill, in black in the daylight presentation, and way labels carry no halo of their own. The dark end of the daylight road palette therefore decides whether a street name can be read: on the motorway fill the black label reaches only about 3.2:1, and on the trunk fill about 2.9:1, where a label of that size needs at least 4.5:1. The same fills are also the darkest colours on the map, so a motorway reads as a heavy line instead of the lightest, most prominent class of the road network.

The constants the style sheets derive from those fills inherit the problem in both directions: the thin fill of a road - the variant drawn below the full road width - is lightened from the fill by a factor that washes it out once the fill itself is light, and the highway shields paint *white* text on a background derived from the fill, so a lighter fill makes the shield text unreadable.

## What Changes

- The daylight fills of the motorway, trunk, primary and secondary road class SHALL be light enough that the black way label drawn on them is readable, and SHALL stay distinguishable from the land and from each other.
- The constants a style sheet derives from a road fill SHALL follow it: the thin fill SHALL stay a usable, less prominent variant of the same class, and a shield background SHALL stay dark enough for the white shield text it carries.
- The dark presentation SHALL keep the fills it has, so the presentation difference stays visible.
- A test SHALL assert the resolved fills of the real style sheets, so the values cannot drift unnoticed.

## Capabilities

### New Capabilities

- `road-fill-contrast`: the daylight road class fills of the style sheets, the constants derived from them, and the readability of the way label and of the shield text drawn on them.

### Modified Capabilities

- None.

## Impact

Affected files:

- `stylesheets/standard.oss` — the daylight fills of the motorway, trunk, primary and secondary class, the thin fills derived from them, and the highway shield and junction label constants derived from them.
- `stylesheets/winter-sports.oss` — the same, for the classes this style sheet shares with the standard style sheet; its warm reds and oranges already sit in the required range.
- `Tests/src/RoadStyleColorsTest.cpp` (new) plus its entries in `Tests/CMakeLists.txt` and `Tests/meson.build` — resolves the road line styles of the two style sheets and asserts the fill of each class, the thin variant, and the stacking of the cased road.
- `stylesheets/include/roads.oss` — consumed, not changed: it draws the roads and consumes the per-style-sheet fill constants; `stylesheets/cycle.oss`, `motorways.oss`, `railways.oss`, `public-transport.oss` and `boundaries.oss` are untouched.
- Consumers: OSMScout2, JavaScout and every other client that loads these style sheets; the daylight map changes in the road classes, the dark presentation does not. No code, API, dependency or database change.
