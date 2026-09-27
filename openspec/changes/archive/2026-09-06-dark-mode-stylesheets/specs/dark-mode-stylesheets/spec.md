# dark-mode-stylesheets Specification

## Purpose

Defines the behavior of the `standard.oss`, `cycle.oss`, and `winter-sports.oss` stylesheets when the `daylight` flag is unset (dark mode): colors render dimmed, buildings and railway remain visible but dimmed, non-essential area fills are hidden, and POI/landmark node icons remain visible. Daylight-mode rendering must stay unchanged.

## ADDED Requirements

### Requirement: Dark mode dims rendered colors

When the `daylight` stylesheet flag is unset, every rendered color that has a daylight value SHALL render darkened relative to that value, and no color SHALL render with full daylight brightness.

#### Scenario: Colors without dark variants are dimmed
- **GIVEN** a stylesheet loaded with the `daylight` flag unset
- **WHEN** the map renders
- **THEN** every color that has a daylight variant SHALL render darkened relative to that variant
- **AND** no color SHALL render with its full daylight brightness

#### Scenario: Derived colors follow their base
- **GIVEN** a color defined as `darken(@base, x)` or `lighten(@base, x)`
- **WHEN** the `daylight` flag is unset
- **THEN** the derived color SHALL be computed from the dimmed base color

### Requirement: Buildings render dimmed in dark mode

When the `daylight` flag is unset, buildings SHALL render dimmed rather than being hidden; minor building types SHALL also render dimmed.

#### Scenario: Generic buildings visible in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders at a magnification where buildings are shown
- **THEN** the `building` type SHALL render with the dark building color
- **AND** the `building` type SHALL NOT be hidden

#### Scenario: Minor buildings visible dimmed in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders at a magnification where minor buildings are shown
- **THEN** minor building types (e.g. garages) SHALL render with the dark minor building color
- **AND** minor building types SHALL NOT be hidden

### Requirement: Railway renders dimmed in dark mode

When the `daylight` flag is unset, railway tracks SHALL render dimmed so level crossings and rail context remain visible.

#### Scenario: Railway tracks visible in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders at a magnification where railway is shown
- **THEN** railway track ways SHALL render with dark track colors
- **AND** railway tracks SHALL NOT be hidden

### Requirement: Non-essential area fills hidden in dark mode

When the `daylight` flag is unset, area fills of the shop, tourism, historic, office, landuse, leisure, and natural categories SHALL NOT render.

#### Scenario: Shop area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** shop area fills SHALL NOT be rendered

#### Scenario: Tourism area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** tourism area fills SHALL NOT be rendered

#### Scenario: Historic area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** historic area fills SHALL NOT be rendered

#### Scenario: Office area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** office area fills SHALL NOT be rendered

#### Scenario: Landuse area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** landuse area fills SHALL NOT be rendered

#### Scenario: Leisure area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** leisure area fills SHALL NOT be rendered

#### Scenario: Natural area fills hidden in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** natural area fills SHALL NOT be rendered

### Requirement: POI and landmark icons remain visible in dark mode

When the `daylight` flag is unset, node icons and labels for POIs and landmarks (monuments, viewpoints, fuel, parking, hospitals, traffic signals) SHALL still render.

#### Scenario: Landmark icons visible in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** node icons of landmark types (e.g. historic monuments, tourism viewpoints) SHALL be rendered

#### Scenario: Navigation-relevant POI icons visible in dark
- **GIVEN** the `daylight` flag is unset
- **WHEN** the map renders
- **THEN** node icons of navigation-relevant POIs (fuel, parking, hospitals, traffic signals) SHALL be rendered

### Requirement: Daylight mode unchanged

When the `daylight` flag is set, rendering SHALL be identical to the pre-change behavior.

#### Scenario: Daylight rendering unchanged
- **GIVEN** the `daylight` flag is set
- **WHEN** the map renders
- **THEN** all colors and fills SHALL render with their daylight values
- **AND** all feature categories SHALL render as before the change

### Requirement: Color consolidation preserves daylight appearance

Colors consolidated into base plus derivation SHALL render identically in daylight mode.

#### Scenario: Consolidated colors render identically in daylight
- **GIVEN** a color consolidated from a literal to `darken(@base, x)` or `lighten(@base, x)`
- **WHEN** the `daylight` flag is set
- **THEN** the consolidated color SHALL render with the same value as the original literal
