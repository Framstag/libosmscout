## Why

The sanitizer configuration documented in `AGENTS.md` enables the AddressSanitizer and links
gperftools' tcmalloc into `Tests/PerformanceTest` (`Tests/CMakeLists.txt:359-362`, the option defaults
to `GPERFTOOLS_USAGE` from `cmake/features.cmake:434`). Both interpose `malloc`/`free`, so the tool's
`free` reaches whichever interposer won the link order for a pointer the other one allocated. A manual
run of that binary therefore reports memory errors that are not the program's: on the fixed view of the
Dortmund database at 300 DPI the OpenGL driver path frees a glib/gobject pointer and the tool reports

```
src/tcmalloc.cc:255] Attempt to free invalid pointer 0x7c1fe7fe0b10
#2 g_slice_free_chain_with_offset () at /usr/lib/libglib-2.0.so.0
#3 g_type_class_get () at /usr/lib/libgobject-2.0.so.0
#7 ??? () at /lib64/ld-linux-x86-64.so.2
```

in **5 of 5 runs** (and **3 of 3** with the source changes of `fix-opengl-area-visibility-cull`
stashed, which is how the conflict was found). The sanitizer job survives it only because its test step
excludes the `PerformanceTest` tests by name; every hand-run of the binary starts with a false
diagnosis, and a real memory error the sanitizer would have found is hidden behind this one.

## What Changes

- A build configuration that enables a sanitizer runtime SHALL provide exactly one allocator
  interposer, so a binary it builds reports the program's own memory errors rather than a conflict
  between two allocators.
- The heap profiling the performance tool offers for ordinary configurations SHALL stay available
  there, and a configuration that disables it for the reason above SHALL say so in its output.
- The Linux sanitizer job SHALL verify the property on the binary it builds, so the conflict cannot
  return unnoticed.
- `AGENTS.md` SHALL state the allocator rule of the sanitizer recipe, because the recipe is meant to be
  run by hand as well.

## Capabilities

### New Capabilities

- `sanitizer-build-configuration`: what a build configuration that enables a sanitizer runtime
  guarantees about the allocator its binaries use, how the heap profiler of the performance tool is
  affected, and how a configuration reports that.

### Modified Capabilities

None. The OpenGL performance test's own capability (`opengl-performance-test`) keeps its requirements:
the tool's shader resolution and registration are not touched, and nothing about how a test is
registered changes.

## Impact

- `cmake/features.cmake` and/or `Tests/CMakeLists.txt` - where the gperftools option is decided and
  consumed; the change is expected to sit where the sanitizer flags are known.
- `.github/workflows/sanitize_on_ubuntu_24_04.yml` - the check on the built binary in the Linux job.
- `AGENTS.md` - the sanitizer build section, which documents the configuration this change repairs.
- `TODO.md` - the finding this change closes, and the rare Release-build symptom that stays
  unexplained (see the proposal of `fix-opengl-area-visibility-cull`'s `verification.md` for its
  evidence).
- Not affected: any library, the public API, the database and file formats, the Meson build (it has no
  gperftools integration at all), and the sanitizer job's existing test exclusions.
