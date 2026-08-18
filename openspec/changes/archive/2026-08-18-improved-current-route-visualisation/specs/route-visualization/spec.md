## MODIFIED Requirements

### Requirement: Route type definitions in stylesheet

The `_route` type is registered at runtime in `TypeConfig::Initialize`; `_route_start` and `_route_end` are registered at runtime by the clients (JNI `withCustomPoiType`, Qt `AddCustomPoiType`). Rendering styles for all three SHALL be defined in `stylesheets/include/route.oss`.

#### Scenario: Route polyline has visible style

- **WHEN** a route is rendered
- **THEN** the `_route` WAY style from `route.oss` is applied as a cased line: a white outline with a red fill
- **AND** the cased line is visible on top of red primary roads

#### Scenario: Route start and end markers render

- **WHEN** a route is rendered
- **THEN** the `_route_start` and `_route_end` NODE.ICON styles from `route.oss` are applied at the route endpoints
