# Spec delta — client-java-avoid-unpaved

## ADDED Requirements

### Requirement: Grade-aware speeds when avoidUnpaved is set

A routing profile created with `avoidUnpaved` SHALL price every routable type of its vehicle with a
per-grade speed table instead of one speed per type: grade 1 (solid) SHALL keep the type's speed, and the
time one kilometre of that type costs SHALL increase strictly from grade 1 to grade 5 while staying finite.
A profile created without the flag SHALL keep one speed for every grade.

#### Scenario: Grade 1 keeps the type's speed
- **GIVEN** a car profile created with `avoidUnpaved`
- **WHEN** one kilometre of a road type at grade 1 is priced
- **THEN** it costs the same as with a profile created without the flag

#### Scenario: The cost rises with the grade
- **GIVEN** a car profile created with `avoidUnpaved`
- **[WHEN]** one kilometre of the same type is priced at grade 1, 2, 3, 4 and 5
- **THEN** the seconds per kilometre increase strictly from grade to grade

#### Scenario: A degraded road stays routable
- **GIVEN** a profile created with `avoidUnpaved`
- **WHEN** the five grade speeds of a routable type are inspected
- **THEN** each of them is greater than zero
- **AND** the profile keeps the junction penalty and the raised cost limit that the vehicle
  parametrization sets, so a route that needs a high-grade way is still calculated instead of reported
  as not found

#### Scenario: Without the flag every grade costs the same
- **GIVEN** a profile created without `avoidUnpaved`
- **WHEN** one kilometre of the same type is priced at grade 1 and at grade 5
- **THEN** both cost the same

#### Scenario: The table covers each vehicle's own routable types
- **GIVEN** a profile created with `avoidUnpaved` for a bicycle or for a pedestrian
- **WHEN** a type that vehicle can route is priced at grade 4
- **THEN** it costs more than the same type at grade 1
- **AND** a type that vehicle cannot route keeps no speed entry
