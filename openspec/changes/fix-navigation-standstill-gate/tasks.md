# Tasks

## 1. The standing decision (spec: navigation-speed-agent)

- [x] 1.1 Keep a window of recent fixes in the agent (`SpeedAgent.h`), separate from the segment history, and document that the segment history is no longer a five-second buffer. Verify: the window is trimmed by fix time and holds positions with their timestamps.
- [x] 1.2 Add the standing gate to the fallback in `SpeedAgent.cpp`: trim the window to five seconds, compare the net displacement between the oldest and the newest fix against a three-metre floor, and apply the decision only once four seconds of history exist. Verify: the decision compares a displacement per window, not the length of a single segment, so it does not depend on the fix rate.
- [x] 1.3 Report the vehicle as standing and discard the segment history while the decision holds. Verify: no speed derived from jitter that arrived before the decision can be published afterwards.
- [x] 1.4 Leave the moving case unchanged: the speed magnitude is still the sum of the segment distances, popped by three seconds of duration. Verify: the existing walking magnitude assertions still hold, and a curve is not reported slower because a displacement cuts the corner.
- [x] 1.5 Reset the fix window together with the segment history at the two existing reset points (a reported standstill and a GPS gap above ten seconds). Verify: a reset cannot leave the gate deciding from fixes that no longer describe the current movement.
- [x] 1.6 Document the resulting limit where the decision is taken: fix jitter above roughly 1.5 m amplitude cannot be separated from a very slow walker, and movement that stays below the floor is reported as standing. Verify: the limit is stated in the code comment and asserted by a test.

## 2. Tests (spec: navigation-speed-agent)

- [x] 2.1 Add `Tests/src/SpeedAgentTest.cpp`: a fixture that drives the agent directly with synthetic fixes (position moving at a given speed, at a given fix rate, with or without jitter), so the tests need neither a database nor a device. Verify: the helper returns the speed published per fix, and NaN when a fix published none.
- [x] 2.2 Cover the jitter contract: a standing vehicle whose fixes jitter by about a metre per second reports standing from the first fix on which the decision applies. Verify: all fixes after the history is full publish exactly a standstill.
- [x] 2.3 Cover that walking is not swallowed: 3, 4 and 5 km/h at 1 Hz, 4 km/h at 0.5 Hz (every fix already covering more than a second), and walking at 1 Hz on top of jitter. Verify: the published speed is close to the real walking speed, and the jitter case is only bounded (the fallback sums segment distances, so its magnitude is inflated).
- [x] 2.4 Cover the boundary the decision cannot see through: movement that accumulates less displacement over the window than the floor (2 km/h at 1 Hz) is reported as standing. Verify: the case fails if the floor is removed, and it documents the floor as a deliberate limit rather than an accident.
- [x] 2.5 Cover the receiver's precedence: a speed reported by the receiver is published unchanged, an implausible reported speed becomes unknown, and a reported standstill keeps the fallback quiet afterwards. Verify: the reported value reaches the listener without the gate being consulted.
- [x] 2.6 Register the new test in `Tests/CMakeLists.txt` and `Tests/meson.build`. Verify: both build systems build and run it.

## 3. Record the pre-existing issue the tests uncovered (spec: navigation-speed-agent)

- [x] 3.1 Record in `TODO.md` that the fallback never runs at fix rates above 1 Hz, because its "segment of at least a second" guard is compared against the previous fix, which is updated for every fix: no speed is published at 4 Hz for a moving or a standing vehicle. Verify: the entry states the measurement, the cause and the shape of a fix, and states that it is not part of this change.

## 4. Build and regression verification (spec: navigation-speed-agent)

- [x] 4.1 Build the core library and the new test in the CMake build and verify they compile without errors and without warnings from the touched files.
- [x] 4.2 Build the same in the Meson build and verify it compiles without errors.
- [x] 4.3 Run the new test through both build systems and verify all cases pass.
- [x] 4.4 Run the rest of the suite and verify no existing test regresses, including the navigation agent tests that exercise the speed messages.
- [x] 4.5 Run `openspec validate "fix-navigation-standstill-gate" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new public behaviour is stated where a client can find it: the capability spec says what a standing vehicle reports and that the receiver's own speed is preferred.
- [x] 5.2 Verify no API, message or build flag changed: the published message types, the agent list of the navigation engine and the Java side are untouched.
- [x] 5.3 Verify the superseded alternative is not carried along: no distance floor per segment exists in the code, because it would report a walker as standing.
- [x] 5.4 Verify the branch contains this change only, and not the route-instruction work that was in progress alongside it.

## 6. Verification evidence

- [x] 6.1 CMake: `ninja SpeedAgentTest` compiles without errors and without compiler warnings from the touched files.
- [x] 6.2 CMake: `SpeedAgentTest` passes 4 cases / 21 assertions, and the full suite passes 91/91 with `--exclude-regex "PerformanceTest"` under `xvfb-run` with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.3 Meson: `meson compile SpeedAgentTest` builds without errors and `meson test "Check speed agent"` passes.
- [x] 6.4 `openspec validate "fix-navigation-standstill-gate" --strict` reports the change as valid.
- [x] 6.5 The diff against master is the speed agent, its new test, the two test build files and the `TODO.md` entry: six files, no unrelated region.
