# javascout-lane-guidance

## Purpose

Provides visual lane guidance in JavaScout so drivers see per-lane turn arrows and suggested lane highlighting during navigation.

## Requirements

### Requirement: Lane guidance overlay in next-turn display

The JavaScout `RoutePanel` SHALL render a lane guidance overlay showing one arrow per lane when lane information is available. The suggested lane(s) SHALL be visually highlighted. The overlay SHALL update whenever `onLaneUpdate` is called.

#### Scenario: Lane arrows shown for multi-lane road
- **WHEN** navigation is active and `onLaneUpdate` is called with `count=3` and `turns=[LEFT, STRAIGHT, RIGHT]`
- **THEN** the next-turn display SHALL show three lane arrows: left, straight, right
- **AND** the suggested lane(s) SHALL be highlighted

#### Scenario: Suggested lane highlighted
- **WHEN** `onLaneUpdate` is called with `suggested=true`, `suggestedFrom=0`, `suggestedTo=1`, `turns=[LEFT, LEFT, RIGHT]`
- **THEN** lanes 0 and 1 SHALL be visually distinct from lane 2

#### Scenario: No lane info available
- **WHEN** `onLaneUpdate` is called with `count=0`
- **THEN** the lane guidance overlay SHALL be hidden

#### Scenario: Lane guidance clears on route end
- **WHEN** navigation reaches the destination
- **THEN** the lane guidance overlay SHALL be cleared

### Requirement: Lane arrow graphics adapt to turn direction

Each lane arrow SHALL visually represent its `LaneTurn` value (e.g., left arrow for `LEFT`, straight arrow for `STRAIGHT_ON`, right arrow for `RIGHT`, slanted arrows for `SLIGHTLY_LEFT`/`SLIGHTLY_RIGHT`).

#### Scenario: Left turn arrow
- **WHEN** a lane has `LaneTurn.LEFT`
- **THEN** its arrow graphic SHALL point left

#### Scenario: Straight on arrow
- **WHEN** a lane has `LaneTurn.STRAIGHT_ON`
- **THEN** its arrow graphic SHALL point straight up

#### Scenario: Combined turn arrow
- **WHEN** a lane has `LaneTurn.LEFT_AND_STRAIGHT`
- **THEN** its arrow graphic SHALL show both left and straight indicators

### Requirement: Lane guidance respects oneway roads

When the road is oneway, the lane arrows SHALL be rendered in the direction of travel without a centre divider.

#### Scenario: Oneway lane display
- **WHEN** `onLaneUpdate` is called with `oneway=true`
- **THEN** the lane arrows SHALL be rendered without a centre divider

### Requirement: LaneTurn Java enum

The system SHALL provide a Java `LaneTurn` enum in package `com.framstag.libosmscout.client` with values matching the C++ `osmscout::LaneTurn` enum.

#### Scenario: LaneTurn values match C++
- **WHEN** the Java `LaneTurn` enum is defined
- **THEN** it SHALL contain at least: `NONE`, `LEFT`, `SLIGHTLY_LEFT`, `STRAIGHT_ON`, `SLIGHTLY_RIGHT`, `RIGHT`, `LEFT_AND_STRAIGHT`, `STRAIGHT_AND_RIGHT`, `LEFT_AND_RIGHT`
