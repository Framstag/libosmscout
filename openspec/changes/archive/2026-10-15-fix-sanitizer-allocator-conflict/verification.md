# Verification

What was run for `fix-sanitizer-allocator-conflict`, on which tree, and what the result was.

## Environment

| item | value |
|---|---|
| sanitizer configuration | `build-asan/` (CMake, Debug, `-fsanitize=address -fsanitize=undefined`, gperftools present) |
| ordinary configuration | `build/` (CMake, Release, gperftools present) |
| second build system | `build-meson/` (Meson, no gperftools integration) |
| reproduction view | `maps/Dortmund`, `stylesheets/public-transport.oss`, zoom 15, 300 DPI, `51.5133 7.4622 - 51.5167 7.4678` |

The reproduction command, run from the configuration's directory:

```bash
./Tests/PerformanceTest-1.1.1 --driver opengl --dpi 300 --start-zoom 15 --end-zoom 15 \
  --shaders ../libosmscout-map-opengl/data/shaders \
  --font ../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf \
  --icons ../libosmscout/data/icons/svg/standard \
  ../maps/Dortmund ../stylesheets/public-transport.oss 51.5133 7.4622 51.5167 7.4678
```

## 1. The before state (task 1.1)

Run in `build-asan` with the tree of `fix-opengl-area-visibility-cull` (the change that found the
conflict), five times:

| run | result |
|---|---|
| 1-5 | all five fail with `src/tcmalloc.cc:255] Attempt to free invalid pointer 0x...`, process ends in `SIGILL` |

```
$ ldd ./Tests/PerformanceTest-1.1.1 | grep -c tcmalloc
1
```

`gdb` on the same command places the free outside the project's code:

```
#0  tcmalloc::Log(...)
#2  g_slice_free_chain_with_offset () at /usr/lib/libglib-2.0.so.0
#3  g_type_class_get () at /usr/lib/libgobject-2.0.so.0
#7  ??? () at /lib64/ld-linux-x86-64.so.2
```

The same command fails 3 of 3 runs with the source changes of `fix-opengl-area-visibility-cull`
stashed, so the conflict is independent of that change; that change's own `verification.md` records
those runs. The Release build of the same configuration (which also links tcmalloc) completes the
command, so the sanitizer runtime in the same binary is what turns the mismatched free into a report.

## 2. The configuration (tasks 2.1, 2.2)

`cmake/features.cmake` now turns `GPERFTOOLS_USAGE` off when the configuration's flags enable a
sanitizer runtime, and says so; the test project's `PERF_TEST_GPERFTOOLS_USAGE` option keeps defaulting
to it, so `-DPERF_TEST_GPERFTOOLS_USAGE=ON` still overrides deliberately.

| configuration | configure output | `ldd` of `Tests/PerformanceTest-1.1.1` |
|---|---|---|
| `build-asan` (sanitizer flags in the cache) | `-- heap profiler (Gperftools) disabled: this configuration enables a sanitizer runtime, whose allocator has to be the only interposer` and `- heap profiler (Gperftools) OFF` | `tcmalloc: 0` occurrences, `libasan: 1` |
| `build` (no sanitizer flags) | `- heap profiler (Gperftools) ON`, no message | `tcmalloc: 1` |
| `build` with `-DPERF_TEST_GPERFTOOLS_USAGE=OFF` | option off | `tcmalloc: 0` |

The stale `PERF_TEST_GPERFTOOLS_USAGE` cache entry of `build-asan` was cleared before the first row
(`cmake -U PERF_TEST_GPERFTOOLS_USAGE build-asan`), because a cache entry wins over an option's new
default: a configuration created before this change keeps the profiler until its cache entry is
removed. The `build` configuration was restored to the profiler after the third row
(`-DPERF_TEST_GPERFTOOLS_USAGE=ON`, `tcmalloc: 1` again).

## 3. The after state (task 2.3)

The reproduction command of section 1, in the rebuilt `build-asan`, five times: **0 of 5 runs fail**,
and no run reports `Attempt to free invalid pointer`.

The same command in the ordinary `build` configuration and in `build-asan` reports the same view:

| report | `build-asan` | `build` |
|---|---|---|
| area rings of the first pass | `examined 115 kept 4` | `examined 115 kept 4` |
| area rings of the second pass | `examined 113 kept 2` | `examined 113 kept 2` |
| loaded data | `nodes: 607 way: 8877 areas: 222` | `nodes: 607 way: 8877 areas: 222` |

So the sanitizer configuration of the tool now reports the program's own memory state on a view that
previously only ever reported the interposer conflict.

## 4. The job and the documentation (tasks 3.1, 3.2, 3.3)

- The Linux sanitizer workflow carries the check in all three of its jobs (`sanitize_clang`,
  `sanitize_clang_memory`, `sanitize_gcc`); the file parses as YAML and each job lists one step named
  "Check that no sanitizer binary carries a second allocator". Locally the same step's commands: pass on
  the rebuilt `build-asan` (`OK: no tcmalloc`), fail when the profiler is forced back into it
  (`FAIL: build-asan/Tests/PerformanceTest-1.1.1 links tcmalloc`), and pass again after clearing the
  cache entry - which is the behaviour the job needs.
- `AGENTS.md` names the rule under the sanitizer recipe: the sanitizer runtime has to be the only
  allocator interposer, the configure output says which profiler it dropped and why, the check is
  `ldd build-asan/Tests/PerformanceTest* | grep tcmalloc`, a sanitizer that arrives through a toolchain
  file or wrapper is not detected (check by hand), and a pre-existing build directory keeps its cached
  option until `cmake -U PERF_TEST_GPERFTOOLS_USAGE <dir>`.
- `TODO.md`: the entry of `fix-opengl-area-visibility-cull` about the heap corruption was rewritten.
  Its reliable reproduction belonged to this change's conflict and is now fixed; what stays is the
  unexplained abort of an *ordinary* `Demos/DrawMapAll` run (two observations in about forty runs, a
  binary that links neither tcmalloc nor a sanitizer, and every attempt against it named).

## 5. Suites and the sanitizer configuration (tasks 4.1, 4.2)

| command | result |
|---|---|
| `cmake --build build -j 4` | no error |
| `xvfb-run -a ctest -j 2 --output-on-failure` in `build/` | **140/140 passed** |
| `meson compile -C build-meson` | no error |
| `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` | **140/140 Ok, 0 Fail** |
| `cmake --build build-asan -j 4` | no error |
| `ctest -j 2 --output-on-failure --exclude-regex PerformanceTest` in `build-asan/` | **94/97 passed**; `MapPainterShieldTest`, `TextMetricsCairoTest` and `TextMetricsSVGTest` fail with their pre-existing `LeakSanitizer` report from the pango/fontconfig path, the same three and the same class as before this change |

So the configuration change leaves the suites as they were, and the sanitizer configuration now fails
only for the reason it already failed for.
