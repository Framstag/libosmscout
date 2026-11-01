# Spec Delta

## ADDED Requirements

### Requirement: Free-text candidate budget counts kept candidates

The per-source free-text candidate budget SHALL count a hit only when the active search scope keeps it, so
a scoped search SHALL collect the requested number of candidates while the index holds them. An
out-of-scope hit SHALL NOT spend the budget; it costs only its own object load. When no scope is active
the budget SHALL be spent on the hits themselves, as before.

#### Scenario: A scoped walk collects the requested candidates

- **GIVEN** a text index that reports more hits than the limit, with out-of-scope hits before the in-scope ones
- **WHEN** a search runs with an active scope and a limit of N
- **THEN** the walk keeps collecting past the out-of-scope hits
- **AND** it delivers N in-scope candidates

#### Scenario: An out-of-scope hit does not spend the budget

- **GIVEN** an active scope and a free-text budget with one candidate still to collect
- **WHEN** a hit outside the scope's extent is offered
- **THEN** the hit is dropped without spending the budget
- **AND** the next in-scope hit still fills the budget

#### Scenario: Without a scope every hit spends the budget

- **GIVEN** no active scope, so the extent admits every position
- **WHEN** the walk offers hits
- **THEN** each hit spends the budget
- **AND** the walk stops once the limit is reached
