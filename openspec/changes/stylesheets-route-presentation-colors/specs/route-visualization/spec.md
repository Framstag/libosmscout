# Spec Delta

## MODIFIED Requirements

### Requirement: Route type definitions in stylesheet

The `_route` type is registered at runtime in `TypeConfig::Initialize`; `_route_start` and `_route_end` are registered at runtime by the clients (JNI `withCustomPoiType`, Qt `AddCustomPoiType`). Rendering styles for all three SHALL be defined in `stylesheets/include/route.oss`.

The route paint SHALL follow the presentation the style sheet is loaded with, and SHALL define a daylight and a dark variant of the fill colour and of the casing colour in that module. The daylight fill SHALL differ from the road colours of the style sheets it is drawn over, and the daylight casing SHALL stay visible on the lightest road fill the style sheets use.

#### Scenario: Route polyline has visible style

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** a route is rendered
- **THEN** the `_route` WAY styles from `route.oss` are applied as a cased line: an opaque violet fill over a dark violet casing
- **AND** the cased line is visible on top of red primary roads

#### Scenario: Route polyline is distinguishable in the dark presentation

- **GIVEN** a style sheet loaded with the dark presentation
- **WHEN** a route is rendered
- **THEN** the `_route` WAY styles are applied as a cased line with the dark variant of the fill and the casing colours
- **AND** the casing of the dark variant SHALL be a light colour, so the route stays visible over the darkened map

#### Scenario: Route start and end markers render

- **WHEN** a route is rendered
- **THEN** the `_route_start` and `_route_end` NODE.ICON styles from `route.oss` are applied at the route endpoints

## ADDED Requirements

### Requirement: Style sheets share the route paint

Every style sheet that can draw an active route SHALL take the route's paint from `stylesheets/include/route.oss` and SHALL NOT define a route line style or a route colour of its own, so that the route looks the same whichever of those style sheets is active and a colour change has one place to be made. A style sheet that includes the module SHALL resolve the same casing and fill styles as the other style sheets that include it.

#### Scenario: Style sheets that draw a route resolve the shared paint

- **GIVEN** the style sheets that can draw an active route
- **WHEN** the route line styles of each are resolved with the same presentation
- **THEN** each of them SHALL resolve the shared module's casing style and fill style
- **AND** the colours SHALL be the same in every one of them

#### Scenario: The cycle style sheet no longer paints its own route

- **GIVEN** the cycle style sheet
- **WHEN** its route line styles are resolved
- **THEN** they SHALL be the casing and the fill of the shared module
- **AND** the style sheet SHALL NOT carry a route colour of its own

#### Scenario: Adding the shared module brings the shared overlay markers

- **GIVEN** a style sheet that includes the shared route module
- **WHEN** the styles of the route start marker, the route end marker, the imported track and the favourite and search-selection markers are resolved
- **THEN** they SHALL be defined by that module, so a style sheet cannot include the route paint without the markers that belong to the same overlay presentation
