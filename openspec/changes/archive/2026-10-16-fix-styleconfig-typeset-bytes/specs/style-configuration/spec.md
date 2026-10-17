# Spec Delta

## Purpose

Defines what loading a stylesheet into a style configuration guarantees: which styles a resolution
returns for an object type at a magnification level, how a reference to a type the type configuration does
not define is handled, and that the cost of the load and the memory the loaded configuration retains
follow the styles the stylesheet references rather than the number of types the type configuration
defines.

## MODIFIED Requirements

### Requirement: Style-configuration build cost is observable

The test suite SHALL include a measurement that reports the build cost of a style configuration for the
shipped stylesheets and the shipped type configuration, so that a regression from type growth becomes
visible. The type-set bytes it reports SHALL be the bytes the retained per-level type sets hold, not an
estimate that repeats the storage representation of a set.

#### Scenario: The measurement reports a figure for the shipped stylesheets

- **GIVEN** the shipped stylesheets and the shipped type configuration
- **WHEN** the measurement runs
- **THEN** it builds a style configuration from them and reports the build cost
- **AND** the reported figures of two consecutive runs are within the tolerance the measurement states

#### Scenario: The reported type-set bytes follow the retained sets

- **GIVEN** a stylesheet and a type configuration, and the same stylesheet with a type configuration
  extended by types the stylesheet does not reference
- **WHEN** the style configuration is built from each
- **THEN** the type-set bytes reported for each are equal

Test case: `StyleConfigLookupCostTest` — "Defined but unreferenced types add no build work"
