# Proposal

## Why

The speed agent falls back to computing a speed from position differences when the receiver reports no
speed of its own. That fallback sums the distances between consecutive fixes, and a standing receiver's fix
jitters around its true position: about one metre of jitter per second-long fix reads as 3.6 km/h, and the
reading persists for as long as the vehicle stands still. A navigation screen therefore claims that a parked
car is moving.

The obvious repair - ignore segments shorter than a couple of metres - deletes walking instead: at 1 Hz a
walker at 3 to 5 km/h covers 0.8 to 1.4 m per fix, less than the jitter of a bad fix, so a distance floor
turns into a speed floor in disguise and reports the walker as standing.

## What Changes

- The agent decides whether the vehicle is standing from the **net displacement** between the oldest and the
  newest fix of a window of up to five seconds, compared against a floor, instead of from the length of a
  single segment: noise does not travel, it stays inside its own radius, while real movement accumulates.
- The decision needs at least four seconds of fix history before it applies, so a start (or a stop) is still
  reported by the plain fallback during that time.
- While the fixes only jitter, the agent reports the vehicle as standing and clears the accumulated segment
  history, so a speed is not published from movement that has ended.
- Only the standing case changes. When the vehicle moves, the reported speed is computed exactly as before
  from the same segment history, so curves, stops and slow movement keep their magnitude.
- The movement floor and the known limit of the decision are stated in the code: fix jitter above roughly
  1.5 m amplitude cannot be separated from a very slow walker, and a receiver that reports its own speed is
  still preferred, so the decision is not consulted then.
- The tests that state this contract are added, together with the pre-existing issue they uncovered while
  being written: the fallback never runs at fix rates above 1 Hz, because its "segment of at least a second"
  guard is compared against the previous fix, which is updated for every fix. That issue is recorded and
  intentionally not fixed here.

## Capabilities

### New Capabilities
- `navigation-speed-agent`: which speed the navigation engine reports to its listeners - the receiver's own
  speed when it has one, otherwise a speed derived from position differences, and how a standing vehicle is
  separated from a moving one.

### Modified Capabilities
<!-- None: no existing capability describes the speed agent's computation. -->

## Impact

Affected files and modules:

- `libosmscout/include/osmscout/navigation/SpeedAgent.h` — the agent keeps a window of recent fixes next to
  the segment history; the segment history's comment no longer claims a five-second bound.
- `libosmscout/src/osmscout/navigation/SpeedAgent.cpp` — the standing gate, the reset points for the new
  window and the unchanged speed computation behind it.
- `Tests/src/SpeedAgentTest.cpp` — new test file driving the agent directly, without a database or a device.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the new test in both build systems.
- `TODO.md` — records the pre-existing fallback issue found while writing the tests.

No change to the GPS input message, the published message types, the navigation engine's agent list or any
build flag, so no client of the agent has to change. No file format, type-config or database impact, and no
`FileFormatVersion.md` version bump applies. No new dependency.

This change is extracted from the downstream branch; the alternative implementation it superseded (zeroing
every segment shorter than two metres) is not carried along, because it deletes walking.
