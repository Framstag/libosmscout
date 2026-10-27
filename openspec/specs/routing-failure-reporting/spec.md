# routing-failure-reporting Specification

## Purpose
Name the reason a route calculation produced no route, and resolve a position to a routable way for the end
of the route it is being resolved for.

## Requirements

### Requirement: A failed calculation names its cause

`RoutingResult` SHALL carry a `RoutingError` that names why the calculation produced no route: `None` (no
failure), `NoStartNode`, `NoTargetNode`, `NoRouteFound`, `DataFailure` or `Aborted`. `Success()` SHALL keep
meaning "the route is not empty", so a caller that only asks whether the calculation worked keeps its
behaviour, and a result nobody failed SHALL report `None`.

#### Scenario: An unattachable end is named

- **GIVEN** a position that resolves to a way which offers no route node reachable for its end
- **WHEN** the calculation returns without a route
- **THEN** the result SHALL report `NoStartNode` for the start and `NoTargetNode` for the target

#### Scenario: An exhausted search is named

- **GIVEN** two positions that are not connected in the loaded routing data
- **WHEN** the calculation returns without a route
- **THEN** the result SHALL report `NoRouteFound`

#### Scenario: A cancellation is not a route failure

- **GIVEN** a calculation whose breaker is aborted
- **WHEN** the calculation returns
- **THEN** the result SHALL report `Aborted`

#### Scenario: A data failure is named

- **GIVEN** a calculation whose routing data cannot be read, or whose result cannot be converted to route data
- **WHEN** the calculation returns without a route
- **THEN** the result SHALL report `DataFailure`

#### Scenario: A calculation that worked reports no error

- **GIVEN** a calculation that produced a route
- **WHEN** its result is inspected
- **THEN** it SHALL report `Success()` true and `GetError()` `None`

### Requirement: The closest routable node is resolved for a named route end

The resolution that picks the closest routable way SHALL take the end of the route it resolves for, named by
a `RouteEndpoint` (`Start`, `Target`, `Unspecified`). It SHALL skip a way the position cannot be attached to
for that end and take the next way within the radius instead of failing the calculation. `Unspecified` SHALL
apply no end-specific restriction.

#### Scenario: A way the end cannot attach to is skipped

- **GIVEN** a position whose closest way offers a route node only in the half of the way the resolved end
  does not search, and a routable way a little further away
- **WHEN** the position is resolved for that end
- **THEN** the further way SHALL be selected
- **AND** the calculation SHALL NOT fail on the unattachable one

#### Scenario: An unnamed end keeps the previous selection

- **GIVEN** the same position and radius
- **WHEN** it is resolved with `Unspecified` and through the resolution that states no end
- **THEN** both SHALL select the same way

#### Scenario: Every way of the radius is considered

- **GIVEN** a radius in which the first candidate way is unusable for the end and a later one is usable
- **WHEN** the position is resolved for that end
- **THEN** the resolution SHALL inspect the later way rather than stopping at the first

### Requirement: A way is attachable as the end-specific searches attach it

Whether a way can carry a position for an end SHALL be read the way the four end-specific route-node searches
read it: the attach index itself first, then along the way in the directions the routing profile permits for
that end — the start searching forward from the attach index, the target backward from it. A way whose only
route node lies in the half the end does not search SHALL NOT be attachable for that end.

#### Scenario: The attach index itself attaches

- **GIVEN** a position whose closest point is a route node of the way
- **WHEN** it is resolved for either end
- **THEN** that way SHALL be attachable for both ends

#### Scenario: A direction-restricted way attaches for one end only

- **GIVEN** a way the profile permits in one direction only, whose route nodes lie behind the attach index
- **WHEN** the way is asked for the end that searches that half
- **THEN** it SHALL be attachable
- **AND** it SHALL NOT be attachable for the other end

### Requirement: The multi-database resolution asks every database for an end-attachable way

The multi-database resolution SHALL pass the end to every loaded database and SHALL take the closest
candidate that can carry the position for that end, so a database whose nearest way is unusable for this end
does not fail the calculation while another database holds a usable one.

#### Scenario: A nearer unusable way does not win

- **GIVEN** two loaded databases, the one with the nearer way unable to carry the position for the end
- **WHEN** the position is resolved for that end
- **THEN** the candidate of the other database SHALL win
- **AND** the calculation SHALL proceed with it

#### Scenario: An unnamed end keeps the previous selection across databases

- **GIVEN** two loaded databases
- **WHEN** the position is resolved with `Unspecified`
- **THEN** the closest candidate overall SHALL be selected, as before the parameter existed

### Requirement: The bridge reports the kind of a failed calculation

A route calculation that produced no route SHALL report, to Java, the kind of failure the routing library
named, together with an English detail and how far the failing end of the route is from the nearest road the
router could use. The translation from the library's reason to the Java kind SHALL be one dependency-free
function of the reason, so a host test can exercise it and the two vocabularies cannot drift.

#### Scenario: An unattachable end reports the distance to the next road

- **GIVEN** a calculation whose start or target cannot be attached
- **WHEN** the failure reaches Java
- **THEN** the reported kind SHALL name that end
- **AND** the reported distance SHALL be how far the failing end is from the nearest usable road, or a
  non-finite value when the router could not measure one

#### Scenario: Every reason maps to a kind

- **GIVEN** each reason the routing library can name
- **WHEN** it is translated
- **THEN** it SHALL map to the kind of the same meaning, and an unnamed reason SHALL map to `KIND_UNKNOWN`

#### Scenario: A cancellation is not reported as a failure

- **GIVEN** a calculation aborted through its breaker
- **WHEN** the result reaches the bridge
- **THEN** it SHALL be reported as a cancellation, not as an error kind

#### Scenario: The distance is unknown where it is not measured

- **GIVEN** a failure that did not happen at an end of the route, or one where the router found no usable
  road to measure against
- **WHEN** the failure reaches Java
- **THEN** the reported distance SHALL be a non-finite value
- **AND** the kind SHALL NOT claim a cause the library did not name

### Requirement: The kind crosses the boundary as a number both sides declare

The failure kinds SHALL be integers whose values are equal on the Java side and in the bridge, because the
integer is what crosses the boundary. The bridge's cause vocabulary SHALL be its own — not the routing
library's enum — so the boundary keeps one translation point.

#### Scenario: The Java constants and the bridge agree

- **GIVEN** the kind constants the Java interface declares
- **WHEN** they are compared with the numbers the bridge sends for the same causes
- **THEN** each pair SHALL be equal
