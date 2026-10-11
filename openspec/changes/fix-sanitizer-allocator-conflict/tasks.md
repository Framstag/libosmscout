# Tasks

## 1. Record the failure the change fixes

- [x] 1.1 Run the reproduction command in the existing `build-asan` configuration and record it as the *before* state in `verification.md`: `Tests/PerformanceTest-1.1.1 --driver opengl --dpi 300 --start-zoom 15 --end-zoom 15 --shaders libosmscout-map-opengl/data/shaders --font libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf --icons libosmscout/data/icons/svg/standard maps/Dortmund stylesheets/public-transport.oss 51.5133 7.4622 51.5167 7.4678`, five runs (spec: `sanitizer-build-configuration` - A manual run of that binary reports no invalid free). Verify: the record names the command, the number of failing runs, the exact message and the `gdb` backtrace of the free.

## 2. The configuration

- [x] 2.1 Make the project's configuration turn the heap profiler off and print the reason when the configuration's compiler or linker flags enable a sanitizer runtime (`cmake/features.cmake`, `Tests/CMakeLists.txt:359-362`) (spec: `sanitizer-build-configuration` - A configuration that enables a sanitizer says which interposer it dropped). Verify: an ASan configure prints the sanitizer as the reason, and a configure without sanitizer flags still reports the profiler as on.
- [x] 2.2 Verify the built binaries: the performance tool of an ASan configuration lists no allocator other than the sanitizer runtime in its dynamic dependencies, and the tool of an ordinary configuration still links the profiler (spec: same requirement, first scenario; and The heap profiler stays available outside sanitizer configurations, first scenario). Verify: `ldd` on both binaries, with the two outputs recorded in `verification.md`.
- [x] 2.3 Run the command of task 1.1 again in the rebuilt ASan configuration and record it as the *after* state in `verification.md`, including the loaded-object and counter report of the view (spec: `sanitizer-build-configuration` - A manual run of that binary reports no invalid free). Verify: no run reports an invalid free, and the tool still reports the view's areas and the painter's examined/kept ring counts.

## 3. The job and the documentation

- [x] 3.1 Add the check to the Linux sanitizer job's build so a second allocator interposer fails the job, with a comment naming the conflict (spec: `sanitizer-build-configuration` - The performance tool of a sanitizer configuration links no second interposer). Verify: the step fails when the profiler is forced back on in a local run of the same command, and passes with the change.
- [x] 3.2 Document the allocator rule of the sanitizer recipe in `AGENTS.md`, including how to see it violated (spec: same requirement; and The heap profiler stays available outside sanitizer configurations). Verify: the section names the rule, the check and the escape hatch of passing the profiler option explicitly.
- [x] 3.3 Update `TODO.md`: remove the entry this change closes if it was recorded as one, and record the rare heap-metadata abort of an ordinary `Demos/DrawMapAll` run as its own entry with everything that was tried against it (two observations in about forty runs, `gdb`, `valgrind` with no error, `MALLOC_CHECK_=3` with `MALLOC_PERTURB_`, and the fact that the tool links no tcmalloc) (spec: `sanitizer-build-configuration` - the Non-Goal of the design). Verify: the entry names the command, the symptom and each attempt.

## 4. Verification

- [x] 4.1 Build both build systems and run both suites: `cmake --build build`, `xvfb-run ctest -j 2 --output-on-failure` in `build/`, `meson compile -C build-meson` and `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs`. Verify: no failure that this change introduced, and the touched CMake files configure and build without warnings.
- [x] 4.2 Run the sanitizer configuration as `AGENTS.md` documents it after the change (`cmake --build build-asan`, `ctest -j 2 --output-on-failure --exclude-regex PerformanceTest`) and record the result in `verification.md`, including the pre-existing text-rendering leak failures if they still appear. Verify: the record names the passed, failed and excluded tests.
