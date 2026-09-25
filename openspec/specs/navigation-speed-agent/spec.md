# navigation-speed-agent Specification

## Purpose
Decide which speed the navigation engine reports while a session is running: the speed the receiver
reports when it has one, otherwise a speed derived from the vehicle's position fixes, with a standing
vehicle reported as standing rather than as the jitter of its own fix.

## Requirements

### Requirement: The speed reported by the receiver is preferred

When a position fix carries a speed reported by the receiver, the agent SHALL publish that speed, converted
to kilometres per hour, for that fix. A speed the receiver reports as a standstill SHALL also clear the
history the agent keeps for the position-difference fallback, so that movement which has ended is not
reported later. A reported speed that is implausible for a road vehicle SHALL be published as unknown
instead of as a value.

#### Scenario: The receiver's speed is published

- **GIVEN** fixes that carry a speed reported by the receiver
- **WHEN** a fix is processed
- **THEN** the agent SHALL publish exactly that speed, converted to kilometres per hour

#### Scenario: An implausible reported speed is unknown

- **GIVEN** a fix whose reported speed is far beyond any road vehicle's speed
- **WHEN** the fix is processed
- **THEN** the agent SHALL publish the speed as unknown rather than as a value

#### Scenario: A reported standstill clears the fallback history

- **GIVEN** a session in which a receiver reported movement and then reports a standstill
- **WHEN** the next fixes arrive without a reported speed
- **THEN** the fallback SHALL NOT publish a speed that comes from the movement that ended

### Requirement: Without a reported speed the agent derives the speed from position differences

When a fix carries no speed reported by the receiver, the agent SHALL derive the speed from the distance and
the time between recent fixes. It SHALL accumulate the history of those fixes and SHALL publish a speed only
once that accumulated history covers a minimum amount of time, so that a receiver which reports faster than
that minimum still produces a speed: the decision SHALL be about the accumulated history and SHALL NOT
require a single interval between two fixes to span the minimum. The derived speed SHALL NOT depend on how
often the receiver reports, so the same movement SHALL read the same at any fix rate. A derived value that is
implausible for a road vehicle SHALL be published as unknown instead of as a value. A gap between fixes large
enough to mean a lost signal SHALL reset the history rather than produce a speed from the position jump.

#### Scenario: A derived speed is published while the vehicle moves

- **GIVEN** fixes without a reported speed that follow a real movement
- **WHEN** the accumulated history covers the minimum
- **THEN** the agent SHALL publish a speed close to the vehicle's real speed

#### Scenario: The derived speed is published at any fix rate

- **GIVEN** the same movement sampled at a slow rate, at the rate of a typical receiver and at a rate well
  above it
- **WHEN** the fixes are processed
- **THEN** each stream SHALL publish a speed close to the same real speed
- **AND** no stream SHALL stay silent for the whole movement, which is what happens when the minimum is
  demanded from a single interval between two fixes

#### Scenario: A standing vehicle whose fixes jitter is reported as standing at a high fix rate

- **GIVEN** a standing vehicle whose fixes jitter and arrive well above once per second
- **WHEN** the accumulated history covers the decision's window
- **THEN** the agent SHALL report the vehicle as standing, exactly as it does at a lower fix rate

#### Scenario: An implausible derived speed is unknown

- **GIVEN** fixes whose positions jump by an implausible distance in the elapsed time
- **WHEN** the speed is derived from them
- **THEN** the agent SHALL publish the speed as unknown rather than as a value

#### Scenario: A long gap resets the history

- **GIVEN** fixes that are separated by more than ten seconds, as in a tunnel or a dropout
- **WHEN** the next fix arrives
- **THEN** the derived speed SHALL NOT be computed across the gap

### Requirement: A standing vehicle is separated from a moving one by net displacement

The agent SHALL decide that a vehicle is standing when the net displacement between the oldest and the newest
fix of a window of recent fixes stays below a distance floor. The decision SHALL require a minimum amount of
fix history before it applies, and it SHALL compare displacement over the window rather than the length of a
single segment, so that it does not depend on how often the receiver reports. While the decision holds, the
agent SHALL report the vehicle as standing and SHALL discard the segment history, so that jitter is not
accumulated into a speed. Movement that accumulates more than the floor of displacement over the window SHALL
be reported as moving, with its speed derived as before. A receiver that reports its own speed SHALL keep the
decision out of the picture for that fix.

#### Scenario: Jitter of a standing vehicle is not reported as speed

- **GIVEN** a standing vehicle whose fixes jitter by about a metre and arrive once per second
- **WHEN** the fixes have been processed long enough for the decision to apply
- **THEN** every following fix SHALL report the vehicle as standing
- **AND** no speed derived from the jitter SHALL be published

#### Scenario: A walker is not swallowed by the decision

- **GIVEN** a vehicle walking at 3, 4 or 5 km/h, which covers less than the jitter of a bad fix per fix
- **WHEN** the fixes have been processed long enough for the decision to apply
- **THEN** the published speed SHALL be close to the real walking speed

#### Scenario: The decision does not depend on the fix rate

- **GIVEN** the same walking movement sampled at a slower rate, so that every fix already covers more than a
  second
- **WHEN** the fixes have been processed long enough for the decision to apply
- **THEN** the published speed SHALL still be close to the real walking speed

#### Scenario: Walking on top of jitter is still movement

- **GIVEN** a vehicle walking at walking speed while its fixes also jitter
- **WHEN** the fixes have been processed long enough for the decision to apply
- **THEN** the published speed SHALL be a moving speed, not a standstill
- **AND** its magnitude MAY be inflated by the jitter, because the fallback sums segment distances

#### Scenario: The decision waits for its history

- **GIVEN** a fresh session whose fixes only jitter
- **WHEN** fewer fixes than the required history have been processed
- **THEN** the decision SHALL NOT apply yet
- **AND** the plain fallback SHALL decide the published speed for those fixes

#### Scenario: Movement slower than the floor is reported as standing

- **GIVEN** a vehicle moving slower than the distance floor over the window, so that it accumulates less
  displacement than jitter of that amplitude would
- **WHEN** the fixes have been processed long enough for the decision to apply
- **THEN** the agent SHALL report the vehicle as standing
- **AND** this limit SHALL be documented where the decision is taken
