# route-description

Turn-by-turn route description display for JavaScout.

## Purpose

Display a columnar turn-by-turn route description below the route parameter box after successful route calculation. Generated via `RouteDescriptionPostprocessor` (matching `Demos/Routing.cpp` pattern). Includes distance, time, street names, turn directions, and waypoint list.

## Requirements

### Requirement: Route description with turn-by-turn instructions

JavaScout SHALL display a turn-by-turn route description below the route parameter box after successful route calculation. The description SHALL be generated using `RouteDescriptionPostprocessor` (matching `Demos/Routing.cpp` pattern) and include: distance, estimated duration, street names, turn directions, and waypoint list. The separate Info toggle button is dropped — route info is part of the description.

#### Scenario: Description shows after route calculation
- **WHEN** a route is successfully calculated
- **THEN** a list of turn-by-turn instructions is displayed below the route parameter box
- **AND** each line is formatted as columnar text with monospace font: total distance | segment distance | total time | segment time | instruction text
- **AND** the first line shows the start instruction with distance 0.0km and time 0:00
- **AND** each subsequent line shows a turn instruction with accumulated distance/time and segment distance/time

#### Scenario: Description updates on recalculation
- **WHEN** a new route is calculated
- **THEN** the description list is replaced with the new route's instructions

#### Scenario: Description clears when route cleared
- **WHEN** the route is cleared
- **THEN** the description list is cleared

### Requirement: Description generation via RouteDescriptionPostprocessor

The route description SHALL be generated in C++ JNI code using `RouteDescriptionPostprocessor::GenerateDescription()` with a custom `Callback` that overrides `BeforeNode(const Node&)` to extract `GetDistance()` and `GetTime()` from each route node.

#### Scenario: Distance and time extracted from route nodes
- **WHEN** the description is generated
- **THEN** `BeforeNode` reads `node.GetDistance().AsMeter()/1000.0` for distance in km and `node.GetTime()` for duration
- **AND** these values are formatted as columns in the output
