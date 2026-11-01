# Spec Delta

## MODIFIED Requirements

### Requirement: Route instruction parser supports optical navigation hints

JavaScout SHALL build each route-list row from the structured route step model (`RouteStep`), not from a route description string: the turn icon, primary text, secondary distance/time, road type and road name SHALL be derived from the step's fields, so a client words and translates a manoeuvre itself and a manoeuvre that carries optical navigation hints can render them from data.

#### Scenario: Parser extracts turn direction, road type, road name, and metadata
- **GIVEN** a route step carrying a turn direction, a road type, a road name and its distance and time
- **WHEN** the route panel builds the row
- **THEN** it SHALL produce a row with turn icon, primary text, secondary distance/time, road type, and road name
- **AND** the structured step SHALL be used to render the instruction card

#### Scenario: Parser handles start and destination lines
- **GIVEN** a route step whose kind is the route start or the destination
- **WHEN** the route panel builds the row
- **THEN** it SHALL assign appropriate start/destination icons and text

#### Scenario: Route instruction cards support keyboard navigation
- **GIVEN** the route panel is expanded and showing instruction cards
- **WHEN** the user presses the Up or Down arrow keys
- **THEN** focus SHALL move between instruction cards
- **AND** the focused card SHALL be visually highlighted

#### Scenario: Route panel closes with Escape key
- **GIVEN** the route panel is expanded
- **WHEN** the user presses the Escape key
- **THEN** the route panel SHALL collapse
