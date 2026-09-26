# Tasks

## 1. Progress reaches the instruction builder (spec: turn-by-turn-instructions)

- [x] 1.1 In `libosmscout/include/osmscout/navigation/RouteInstructionAgent.h`, route both `GenerateNextRouteInstruction` call sites through one private helper that passes `position.abscissa` when the builder accepts a four-argument call (C++20 `if constexpr (requires { ... })`) and otherwise calls the existing three-argument form unchanged. Verify: the file compiles as part of the library build.
- [x] 1.2 In `Tests/src/NavigationAgentTest.cpp`, extend the position-message helper so a test position can carry progress, and add a progress-aware builder that records the value it was given. Verify: the new test case (`NavigationAgentTest`) passes and the existing coordinate-only `TestInstructionBuilder` cases still pass unchanged.

## 2. Java instruction distances (spec: turn-by-turn-instructions)

- [x] 2.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, change the Java instruction builder's next-instruction call to accept the position's progress, compute the travelled distance as the current segment's length times that progress (clamped to the segment length), and keep the straight-line estimate when the progress is zero. Verify: the native library compiles and no existing behaviour changes for a builder called without progress.
- [x] 2.2 In `libosmscout-client-java/src/OSMScoutClient.cpp`, make `OnTargetReached` report the distance of the destination node from the route start instead of 0. Verify: the value matches `OnStart`'s rule, and the next-instruction search can find the arrival instruction in the collected list.
- [x] 2.3 Verify the two JNI changes together on a real route. Verify: the Java test added in 3.2 reports a non-empty arrival instruction with a distance greater than 0 and a shrinking next-instruction distance.

## 3. Tests

- [x] 3.1 Verify that the core navigation agents keep their contract. Verify: `cd build && ctest -R NavigationAgentTest --output-on-failure` passes, including the new progress case and the unchanged coordinate-only cases.
- [x] 3.2 Add a JUnit 5 test in `JavaScout/src/test/java/com/framstag/libosmscout/client/` for the Java-visible distances, using the routable map directory convention of the existing navigation tests (`nav.test.db.dir` / `JAVASCOUT_MAP_DIR`) and skipping with an assumption when it is not set: the arrival instruction is delivered after the last manoeuvre with a distance greater than 0 and at most the route distance, the arrival instruction in the full instruction list carries the route's overall distance within tolerance, and the next-instruction distance shrinks as successive fixes advance along the same segment without ever becoming negative. Verify: `cd JavaScout && mvn test -Dnative.lib.dir=<build>/lib -Dtest=OSMScoutClientInstructionDistanceTest` passes with a routable map directory set and reports a skip without one.
- [x] 3.3 Verify that all pre-existing client and application tests still pass. Verify: `cd JavaScout && mvn test -Dnative.lib.dir=<build>/lib` reports no new failures.
- [x] 3.4 Verify that the C++ test suite still passes. Verify: `cd build && ctest -j 2 --output-on-failure` reports no new failures.

## 4. Build and quality gates

- [x] 4.1 Verify the full build compiles without errors and without new warnings for the affected targets. Verify: the CMake build (`cmake --build build`) and, where available, the Meson build (`meson compile -C build-meson`) both complete.
- [x] 4.2 Verify the changed C++ and header files against the project rules. Verify: `clang-tidy` with the repository `.clang-tidy` reports no new findings for `RouteInstructionAgent.h`, `OSMScoutClient.cpp` and `Tests/src/NavigationAgentTest.cpp`; note whether `./scripts/format-check.sh check` flags the files, and whether it already flags them on `master` (the script flags most of the repository, so pre-existing drift is the baseline). **Result:** clang-tidy reports 691 findings for `OSMScoutClient.cpp` (baseline 693) and 28 for `NavigationAgentTest.cpp` (baseline 30), with no finding in any added region, and the two `RouteInstructionAgent.h` findings are the pre-existing `inline` specifiers at lines 35 and 46 - zero new findings. Formatting: `NavigationAgentTest.cpp` is uncrustify-clean on `master` (0 drift) and was reformatted back to 0 drift after this change; the header (189 -> 229) and `OSMScoutClient.cpp` (6987 -> 6996) carry the repository's pre-existing uncrustify drift, which the script reports for 799 files including unmodified ones and which no workflow enforces.
- [x] 4.3 Update the documentation affected by the contract change. Verify: `libosmscout-client-java/AGENTS.md` describes the progress-aware instruction distance and the arrival instruction's distance, and mentions nothing that contradicts `RouteInstructionAgent.h`.
