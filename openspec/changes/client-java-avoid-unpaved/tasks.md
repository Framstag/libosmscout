# Tasks — client-java-avoid-unpaved

## 1. Measurement (spec: Grade-aware speeds when avoidUnpaved is set)

- [x] 1.1 Build a routable database and census its routing index.
- [x] 1.2 Measure the seconds per kilometre of one type at grades 1..5, with and without the flag.
- [x] 1.3 Measure candidate grade tables and reject the ones that make a calculation fail with no route.
- [x] 1.4 Record the run as `evidence/avoid-unpaved-probe-<date>.log`.

## 2. Bridge (spec: Grade-aware speeds when avoidUnpaved is set)

- [x] 2.1 Hoist the car speed map and the resolved vehicle speed out of the vehicle switch, so the graded
  pass can reuse them for every vehicle.
- [x] 2.2 Add the graded speed pass: parametrize the vehicle first, then re-price every routable type with
  the per-grade table and its floor.
- [x] 2.3 Replace the log-only `avoidUnpaved` branch with the call and a log line that names the applied
  table and floor.
- [x] 2.4 Compile the translation unit.

## 3. Specification

- [x] 3.1 Add the capability's scenarios to `specs/routing-profile/spec.md`.
- [x] 3.2 Run `openspec validate client-java-avoid-unpaved --strict`.

## 4. Evidence owed before archive

- [x] 4.1 Re-run the probe against the changed bridge end to end through `calculateRouteAsync` with
  `avoidUnpaved = true` and store the log beside the first one:
  `evidence/AvoidUnpavedBridgeProbe.java` -> `evidence/avoid-unpaved-bridge-probe-<date>.log`. The log
  shows the table line and a route for both flag values; the route itself is unchanged (9 122 m, 312
  points) because the graded roads are not on that route, which is what the C++ grid measured too.
- [ ] 4.2 Find a route whose only option crosses a way carrying a grade above 1 under a *car-routable
  type* (a road with `surface`/`tracktype`, not a track — tracks carry no speed in the car profile), and
  confirm it is still calculated with `avoidUnpaved = true`. The Bremen extract has none: across the
  C++ probe's 32 routable pairs no table changed a route. This needs a rural extract.
