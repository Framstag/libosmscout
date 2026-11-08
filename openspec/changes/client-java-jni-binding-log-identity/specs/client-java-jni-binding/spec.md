# Spec Delta

## ADDED Requirements

### Requirement: A bridge log line carries no coordinate

The JNI bridge of the Java client SHALL NOT write a coordinate pair to a log line: no `GeoCoord::GetDisplayText()` or `GeoBox::GetDisplayText()` value SHALL be a logging argument of the bridge. A log line that reports a database check, the outcome of a routable-node lookup, or the resolution of a route end SHALL carry precision-free identity instead — the database path, a count, a distance, or the name of the route end.

#### Scenario: A database check logs the database, not its box

- **GIVEN** the legacy route entry iterating over the open databases
- **WHEN** it tests the route coordinate against a database's bounding box
- **THEN** the trace line names that database's path
- **AND** no coordinate pair appears in the line

#### Scenario: A successful lookup logs the radius, not the coordinate

- **GIVEN** a `GetClosestRoutableNode` lookup that succeeded within its search radius
- **WHEN** the bridge traces the success
- **THEN** the line names the radius it searched within
- **AND** no coordinate pair appears in the line

#### Scenario: A route end is traced by name

- **GIVEN** the start or the destination position being resolved
- **WHEN** the bridge traces the attempt or the failure
- **THEN** the line names the route end (start or destination)
- **AND** no coordinate pair appears in the line
