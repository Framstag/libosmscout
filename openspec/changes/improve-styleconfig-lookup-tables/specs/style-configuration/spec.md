# Spec Delta

## Purpose

Defines what loading a stylesheet into a style configuration guarantees: which styles a resolution
returns for an object type at a magnification level, how a reference to a type the type configuration does
not define is handled, and that the cost of the load and the memory the loaded configuration retains
follow the styles the stylesheet references rather than the number of types the type configuration
defines.

## ADDED Requirements

### Requirement: Style resolution of a referenced type is unchanged

Resolving a style for an object type at a magnification level SHALL return exactly the styles the loaded
stylesheet declares for that type at that level, with the same visibility, priority and composition as
before this change.

#### Scenario: A referenced type resolves its declared styles

- **GIVEN** a type configuration and a stylesheet that declares an area fill and an area text rule for
  one type, both active at the tested magnification level
- **WHEN** the fill style and the text styles of that type are resolved at that level
- **THEN** the resolved fill style is the one the stylesheet declares
- **AND** the resolved text styles are the ones the stylesheet declares

#### Scenario: A rule with a minimum level resolves nothing below it

- **GIVEN** a stylesheet whose rule for one type becomes active at a higher magnification level than the
  tested one
- **WHEN** the style of that type is resolved below that level
- **THEN** no style is resolved for that type

#### Scenario: Two rules for one type compose as declared

- **GIVEN** a stylesheet with two rules for one type at the tested level, one adding an attribute the
  other does not set
- **WHEN** the style of that type is resolved at that level
- **THEN** the resolved style carries the attribute of the second rule and the value of the first

#### Scenario: Rules that name no type still cover every type

- **GIVEN** a stylesheet with a rule that carries no type selector, next to rules that do
- **WHEN** the style configuration is built and the type-less rule is resolved for an arbitrary defined type
- **THEN** the type-less rule is applied to that type
- **AND** the rules that name types only apply to the types they name

### Requirement: A type the stylesheet does not reference resolves to no style

Resolving a style for an object type that the loaded stylesheet does not reference SHALL return no style,
in a debug build as well as in a release build, and SHALL NOT abort, assert or read past the end of a
lookup structure.

#### Scenario: Unreferenced type yields no style without aborting

- **GIVEN** a type configuration with a type that the loaded stylesheet never mentions
- **WHEN** the styles of that type are resolved at a magnification level
- **THEN** no style is resolved for it
- **AND** the resolution completes without an assert failure in a debug build

#### Scenario: Unreferenced and referenced types share one configuration

- **GIVEN** one loaded style configuration and two types, one referenced by the stylesheet and one not
- **WHEN** both types are resolved against that same configuration
- **THEN** the unreferenced type resolves to no style
- **AND** the referenced type still resolves its declared style

### Requirement: Style-configuration build cost follows the referenced styles

The work of building a style configuration from a stylesheet SHALL be governed by the types the stylesheet
references and the magnification levels it uses, and SHALL NOT grow with the number of types the type
configuration defines beyond those referenced.

#### Scenario: Defined but unreferenced types add no build work

- **GIVEN** a stylesheet and a type configuration, and the same stylesheet with a type configuration
  extended by types the stylesheet does not reference
- **WHEN** the style configuration is built from each
- **THEN** the number of prepared style-selector slots reported by the configuration is the same for both
- **AND** the number of type-condition evaluations reported for the build is the same for both, because the
  build walks the types the stylesheet's rules name rather than every defined type
- **AND** neither number grows with the number of defined types

#### Scenario: A stylesheet referencing more types prepares more slots

- **GIVEN** two stylesheets against the same type configuration, one referencing more types than the other
- **WHEN** the style configuration is built from each
- **THEN** the configuration of the larger stylesheet reports more prepared style-selector slots

### Requirement: Retained memory of a loaded style configuration follows the referenced styles

The memory a loaded style configuration retains for style resolution SHALL be governed by the referenced
types and their levels, not by the number of types the type configuration defines.

#### Scenario: Types the stylesheet does not reference retain no lookup entries

- **GIVEN** a loaded style configuration built from a stylesheet that references a subset of the defined
  types
- **WHEN** the lookup structures of the configuration are inspected for a defined but unreferenced type
- **THEN** no lookup entry is retained for that type

#### Scenario: Growth of the defined type set does not grow the retained structures

- **GIVEN** two type configurations that differ only by types the stylesheet does not reference
- **WHEN** both are loaded with the same stylesheet
- **THEN** the retained lookup structures have the same size

### Requirement: A stylesheet that references an undefined type still loads

A stylesheet whose rules reference a type the type configuration does not define SHALL load successfully,
SHALL report the unresolved reference as it does today, and SHALL still serve the rules of the types it can
resolve.

#### Scenario: Undefined reference is reported and the remaining rules resolve

- **GIVEN** a stylesheet with one rule for a type the type configuration does not define and one rule for
  a type it does define
- **WHEN** the stylesheet is loaded into a style configuration
- **THEN** the load succeeds and the unresolved reference is reported
- **AND** the rule of the defined type resolves its declared style

### Requirement: The style-resolution accessor contract is preserved

The public accessors through which a painter resolves a style for an object or a type SHALL keep their
signatures and their results, so that no renderer backend has to be adapted.

#### Scenario: Renderer backends compile and pass unchanged

- **GIVEN** the renderer backends that resolve styles through the style configuration
- **WHEN** the library is built with those backends enabled and the test suite is run
- **THEN** no backend needs a change to its style-resolution calls
- **AND** the existing test suite passes

### Requirement: Style-configuration build cost is observable

The test suite SHALL include a measurement that reports the build cost of a style configuration for the
shipped stylesheets and the shipped type configuration, so that a regression from type growth becomes
visible.

#### Scenario: The measurement reports a figure for the shipped stylesheets

- **GIVEN** the shipped stylesheets and the shipped type configuration
- **WHEN** the measurement runs
- **THEN** it builds a style configuration from them and reports the build cost
- **AND** the reported figures of two consecutive runs are within the tolerance the measurement states
