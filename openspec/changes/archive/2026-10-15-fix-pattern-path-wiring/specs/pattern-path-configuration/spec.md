# Spec Delta

## Purpose

Defines the contract a rendering entry point must satisfy when the stylesheet it renders uses pattern
fills: the pattern image sources are configured, a render that cannot serve a pattern says so instead of
silently degrading, and the patterns the shipped stylesheets reference exist as shipped images.

## ADDED Requirements

### Requirement: Rendering entry points configure pattern image sources

An entry point that renders a stylesheet SHALL configure the directories searched for pattern images
whenever it configures the directories searched for icon images, for every style set it renders.

#### Scenario: Demo entry point renders pattern fills

- **WHEN** an in-repo demo tool renders a stylesheet that uses pattern fills
- **THEN** the pattern image directories are configured before the first render
- **THEN** an area with a pattern fill is filled with the pattern image and not with its solid fallback colour

#### Scenario: Command-line tool offers the pattern directory

- **WHEN** an in-repo tool that renders a stylesheet parses its arguments
- **THEN** it accepts a pattern image directory the same way it accepts an icon image directory

#### Scenario: Entry point without icon directories

- **WHEN** an entry point is used without any image directory configured
- **THEN** its behaviour is unchanged from before this change apart from the reported condition below

### Requirement: A pattern fill that cannot be served is reported

A render of a backend that implements pattern fills SHALL report a pattern fill that no configured
pattern image source can serve, before it draws the solid fallback colour, and SHALL name the unresolvable
pattern together with the pattern image sources it considered - the directories searched, or the statement
that none is configured. A stylesheet that uses pattern fills and a render that has no configured pattern
image source SHALL be reported as a misconfiguration rather than as a missing file.

#### Scenario: No pattern image source configured

- **WHEN** a stylesheet that uses pattern fills is rendered with no pattern image directory configured
- **THEN** the render reports that no pattern image source is configured, naming the pattern that could
  not be resolved
- **THEN** the area is drawn with the solid fallback colour

#### Scenario: Configured source does not contain the pattern

- **WHEN** a pattern image directory is configured but holds no image for a pattern the stylesheet uses
- **THEN** the render reports the unresolvable pattern and the directories searched
- **THEN** the area is drawn with the solid fallback colour

#### Scenario: Report happens once per pattern

- **WHEN** several areas use the same pattern that cannot be served
- **THEN** the condition is reported once for that pattern during the render and not once per area

#### Scenario: Served pattern is not reported

- **WHEN** a pattern image is found in a configured directory
- **THEN** no report is produced for that pattern

### Requirement: Shipped pattern references resolve to shipped images

The stylesheets shipped with the library SHALL be checkable against the image directories shipped with the
library, and the check SHALL name every pattern reference that no shipped image satisfies.

#### Scenario: Every shipped pattern reference resolves

- **WHEN** the check runs over the shipped stylesheets and the shipped image directories
- **THEN** it reports no unresolved pattern reference

#### Scenario: Unresolved pattern reference fails the check

- **WHEN** a shipped stylesheet references a pattern name that no shipped image satisfies
- **THEN** the check fails and names the pattern name and the stylesheet that references it

#### Scenario: Check is part of the test suite

- **WHEN** the test suite is built by either build system
- **THEN** the check is registered and runs as a test of that build system
