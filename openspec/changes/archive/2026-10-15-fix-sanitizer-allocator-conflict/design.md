# Design

## Context

See `proposal.md` for the motivation. This is the state the approach works with.

- The heap profiler is a CMake-only option of the test project: `cmake/features.cmake:434-436` sets
  `GPERFTOOLS_USAGE` from the found gperftools, and `Tests/CMakeLists.txt:359-362` turns that into
  `PERF_TEST_GPERFTOOLS_USAGE` (defaulting to it) and links `${GPERFTOOLS_LIBRARIES}` into
  `PerformanceTest`. The Meson build has no gperftools integration at all, so only CMake is affected.
- The sanitizer recipe in `AGENTS.md` passes the sanitizer through the caches
  (`-DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"`, `-DCMAKE_EXE_LINKER_FLAGS=...`) and
  does not touch the profiler option, so the sanitizer build links tcmalloc and the AddressSanitizer
  runtime into one binary. Both interpose the allocator; the resulting invalid free is reported from
  the tool's own runtime and its stack is in glib/gobject, reached through the dynamic loader, which is
  that library family's first-use path on the OpenGL driver's way in.
- The sanitizer workflow runs its test step with `--exclude-regex "PerformanceTest"` (the UI libraries
  leak on exit), so the job never executes the tool and the conflict is invisible there. The tool is
  built in the job, which is what makes a hand run reproduce it.
- The repository's `Tests/` are not built by the sanitizer job's test step only; the binary exists in
  the build directory of the sanitizer configuration, which is the documented way to run it.

## Goals / Non-Goals

**Goals:**

- A sanitizer configuration builds binaries whose only allocator interposer is the sanitizer's.
- An ordinary configuration keeps the profiler, and the reason for its absence is printed where it is
  absent.
- The property is checked in the job, not only by hand.
- The rule is written down where the sanitizer configuration is documented.

**Non-Goals:**

- The rare heap-metadata abort of an ordinary `Demos/DrawMapAll` run on the same view (two observations
  in about forty runs, reproduced by no tool, no project frame in the report). It stays a `TODO.md`
  entry with its evidence.
- The gperftools support in a sanitizer configuration being *possible* to ask for deliberately; the
  change makes the default safe, not the option unreachable.
- The sanitizer job's exclusion of the `PerformanceTest` tests and the pango/fontconfig leaks that
  justify it.

## Decisions

### D1 - The configuration drops the profiler when its flags enable a sanitizer

| | approach | rationale |
|---|---|---|
| a | **chosen** - the project's configuration detects a sanitizer in the flags it was given and turns the profiler off with a message naming the reason | the recipe in `AGENTS.md` and any hand-run configuration get the safe default without editing the workflow; the option stays for the configurations that want it |
| b | pass `-DPERF_TEST_GPERFTOOLS_USAGE=OFF` in the sanitizer workflow's configure step | surgical and one line, but the trap stays for every hand-configured sanitizer build, which is exactly how the conflict was found, and `AGENTS.md` documents hand configuration |
| c | remove the gperftools integration | removes the false diagnosis permanently, but takes a developer feature away from ordinary builds and does not fit the project's option handling |

Detection reads the sanitizer switches from the configuration's compiler and linker flags. Risks: a
sanitizer enabled through a toolchain file or a compiler wrapper is not seen - documented in
`AGENTS.md` together with the check that reveals it (`ldd` on the binary, or the scenario of the spec);
and a user who wants the profiler next to a sanitizer can still pass the option explicitly, which is
their deliberate choice.

### D2 - The property is checked on the built binary in the job

| | approach | rationale |
|---|---|---|
| a | **chosen** - a step in the Linux sanitizer job asserts that the built performance tool has no allocator other than the sanitizer's | the observable is the linked library; the check is cheap, needs no display and fails the job, so the conflict cannot return unnoticed |
| b | assert the linked libraries at configure or build time with a generator expression | catches it earlier, but the check would live in the build description of every configuration and has to be spelled for each platform's linker output |
| c | document it in `AGENTS.md` only | no gate at all; the conflict already survived one configuration change unnoticed |

Risks: the check reads the dynamic dependencies, which differ per platform, so it is added to the Linux
job only (the configuration the recipe documents); a Windows or macOS sanitizer configuration would
still need its own check, which is why the rule is also written in `AGENTS.md`.

### D3 - The sanitizer job keeps excluding the performance tests

| | approach | rationale |
|---|---|---|
| a | **chosen** - keep the exclusion and say why in the job | the exclusion exists because the UI libraries leak on exit; this change does not touch the leak, and running a performance test in the sanitizer job would add a display requirement and a timing-sensitive gate |
| b | run the performance tests in the sanitizer job once the interposer conflict is gone | plausible now, but it couples two findings, and the pango/fontconfig leak would fail the run as soon as leak detection is on |
| c | drop the tool from the sanitizer configuration with a build option | hides the binary a hand run wants to use and removes the coverage the job's build provides |

## Sequence diagram

```text
BEFORE - the sanitizer configuration

  configure  -fsanitize=address           find gperftools
             ---------------------------> GPERFTOOLS_USAGE=ON
  build      PerformanceTest links:  AddressSanitizer runtime  +  tcmalloc
  run        program frees a pointer          which allocator owns the pointer?
             ---------------------------> tcmalloc: "Attempt to free invalid pointer"
                                          (glib/gobject frame, loader frames)
  conclusion false: the report is about the two interposers, not the program

AFTER - the sanitizer configuration

  configure  -fsanitize=address           sanitizer seen in the flags
             ---------------------------> profiler disabled, reason printed
  build      PerformanceTest links:  AddressSanitizer runtime only
  run        program frees a pointer          one interposer owns every pointer
             ---------------------------> the program's own errors, and only those

AFTER - an ordinary configuration

  configure  no sanitizer flags           find gperftools
             ---------------------------> profiler stays enabled
  build      PerformanceTest links:  tcmalloc (the profiler's allocator)
```

## Risks / Trade-offs

- [The profiler disappears from a configuration the user expected it in] -> the configure step names
  the profiler and the reason; the option can be passed explicitly to override, and `AGENTS.md` states
  both.
- [The detection misses a sanitizer that arrives through a toolchain file or wrapper] -> the spec's
  observable is the built binary, the `AGENTS.md` rule documents the check, and the job's step covers
  the recipe this repository uses.
- [The platform-specific check in the job reads like a magic command] -> it is added next to the test
  step with a comment naming the conflict it guards against.

## Migration Plan

None. No format, database or API change; the effect is confined to how a configuration is built.
Rollback is a revert, which restores the conflict for sanitizer configurations only.

## Open Questions

None that would change the specs, the approach or the task breakdown.
