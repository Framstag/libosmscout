# Tasks

## 1. Time formatting (spec: route-description)

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, change the time part of the description text
  builder: whole hours and remaining whole minutes from an hour, whole minutes from a minute, whole seconds
  from a second, and no time part below a second. Verify: the distance part and the separator rule are
  unchanged. The rule itself lives in the new dependency-free header
  `libosmscout-client-java/src/route_step_time.h`, which both builders call; the distance part is copied
  unchanged and the comma now depends on the same optional time text the line prints.
- [x] 1.2 Apply the same rule in the second description path (the async calculate / reroute path). Verify: the
  two paths produce the same time column for the same description. Both `AppendDistanceTime` bodies of
  `DescCallback` now call `naviveylin::FormatRouteStepTime`, so they cannot drift; the comparison of two real
  line lists is covered by construction (see `verification.md` section 4).
- [x] 1.3 Add a comment at the rule naming the observed defect it fixes, so the sub-minute branch is not
  "simplified" back to minutes.

## 2. Verification (spec: route-description)

- [x] 2.1 Add a Java test for the time column of a description line: a step of minutes, a step of hours and
  minutes, a step of seconds below a minute, and a step below a second. Verify: if the formatting is factored
  into a helper, the test is a host test; otherwise it needs a routable map and reports a skip without one.
  **The host-test branch was taken**: the formatting was factored into `route_step_time.h` and the host test
  `Tests/src/RouteStepTimeTest.cpp` covers it, registered in `Tests/CMakeLists.txt` and `Tests/meson.build`
  with the `libosmscout-client-java/src` include path, like `SearchScopeTest`. No map database is involved.
- [x] 2.2 Verify the two description paths agree for one route: the time column of corresponding lines is
  equal. Verify: the test compares the calculate and the recalculation output. Not host-testable without a
  routable map database (the shipped ones are format version 26, the library expects 27), so covered by
  construction: one shared helper produces the time text of both paths. Recorded in `verification.md`.
- [x] 2.3 Verify nothing else changed: the per-step numeric values, the distance column and the other columns
  of the line are unchanged. Verified by inspection of the diff: only the time part and the separator's time
  condition change; `JavaRouteInstruction.timeTo` and the rest of the instruction stay in `CollectCallback`.
- [x] 2.4 Build the CMake build with the Java client library and the jar, and verify it compiles without
  errors and without warnings from the touched file.
- [x] 2.5 Build the Meson build the same way and verify it compiles without errors.
- [x] 2.6 Run the Java and C++ suites and verify no existing test regresses.
- [x] 2.7 Run `openspec validate "client-java-route-instruction-time-format" --strict` and verify the change
  validates.

## 3. Documentation and change hygiene

- [x] 3.1 Verify the printed rule is documented where a consumer reads the description lines (the route
  description capability and, if present, the client's own documentation of the line format). The rule is in
  the `route-description` spec delta and in the header's doc comment; the client has no separate documentation
  of the line format (`RouteEntry.java` only names the list), so nothing else had to change.
- [x] 3.2 Verify the diff touches only the description text builder in
  `libosmscout-client-java/src/OSMScoutClient.cpp` and, if added, a test. The diff touches the two
  `AppendDistanceTime` bodies and the include list of `OSMScoutClient.cpp`, plus the new header, the new test
  and the two test registrations.
