# route-rendering

Core route rendering in the MapPainter pipeline: guarantees the active route is painted above all map ways and supports multiple stacked line styles (casing) per route.

## Purpose

Core route rendering in the MapPainter pipeline: guarantees the active route is painted above all map ways and supports multiple stacked line styles (casing) per route.

## Requirements

### Requirement: Route drawn above all map ways

The active route SHALL be painted above all other ways in the map, including ways with positive OSM layer values such as bridges and elevated roads.

#### Scenario: Route renders above bridge

- **WHEN** a route crosses a bridge (a way with layer > 0)
- **THEN** the route polyline is painted on top of the bridge

#### Scenario: Route renders above tunnel

- **WHEN** a route passes through a tunnel (a way with layer < 0)
- **THEN** the route polyline is painted on top of the tunnel

#### Scenario: Route renders above normal roads

- **WHEN** a route follows a normal road (layer = 0)
- **THEN** the route polyline is painted on top of the road

### Requirement: Route supports multiple line styles

A route SHALL be rendered with all line styles that match its type in the stylesheet, stacked in slot order, so that casing (outline + fill) can be expressed in the stylesheet.

#### Scenario: Cased route renders outline and fill

- **WHEN** the stylesheet defines an outline rule and a fill rule for the route type
- **THEN** the outline is painted first (below) and the fill is painted on top of it

#### Scenario: Single style route unchanged

- **WHEN** the stylesheet defines a single line style for the route type
- **THEN** the route renders with that single style, as before this change
