## ADDED Requirements

### Requirement: Lane guidance sub-component in next-turn display

The JavaScout `RoutePanel` next-turn display SHALL include a lane guidance sub-component that renders lane arrows when lane information is available. The sub-component SHALL be positioned below the turn icon and distance.

#### Scenario: Lane guidance shown in next-turn area
- **WHEN** `onLaneUpdate` is called with valid lane data during an active navigation session
- **THEN** the next-turn display SHALL show lane arrows below the turn icon and distance

#### Scenario: Lane guidance hidden when no lane data
- **WHEN** `onLaneUpdate` is called with `count=0`
- **THEN** the lane guidance sub-component SHALL be hidden
- **AND** the next-turn display SHALL retain its existing layout
