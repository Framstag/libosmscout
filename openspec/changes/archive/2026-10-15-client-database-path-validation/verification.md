# Verification: client-database-path-validation

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Host: Linux. Two pre-existing Ninja build directories were used: `build/` (CMake, Release,
  `OSMSCOUT_BUILD_CLIENT=ON`, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`, `OSMSCOUT_BUILD_TESTS=ON`) and
  `build-meson/` (Meson). The Meson tree needed no reconfigure: no build description changed.
- The CTest suite was run under `xvfb-run -a`; the suite sets `QT_QPA_PLATFORM=offscreen` itself.
- Only explicit build targets were built (`DatabaseOpenTest`, `osmscout_client_java`,
  `libosmscoutclientjava`), never the default CMake target.
- The ported reference implementation is `origin/naviveylin-local`, commits `041996945` (registry and
  tests) and `fd1b5b378` (Java bridge). Master already carried the earlier `database-path-registry`
  change, so the port is additive there.

## 1. The predicate and the validating entry point

`libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h` gains the free function
`bool IsOpenableDatabaseDirectory(const std::filesystem::path &path) noexcept` and the member
`bool RegisterOpenable(const std::filesystem::path &path)`. The predicate's documentation states its
question ("the path exists and is a directory"), the `noexcept` contract (any filesystem error is a
"no"), that a symbolic link to a directory counts, that it is the client's only path check, and that only
`RegisterOpenable()` acts on its answer - the batch is deliberately not decided by it.

`libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp` implements the predicate with the
error-code overload of `std::filesystem::is_directory(path, error)` (so a permission failure or an
unmounted path cannot throw), and `RegisterOpenable()` asks it **before** taking `mutex_` and then calls
`Register(path)`. A rejected path therefore enters no set and counts no set change.

Call sites of the predicate outside the tests (grep over `*.h`, `*.cpp`, `*.java`):

```
libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp:56:  if (!IsOpenableDatabaseDirectory(path)) {
libosmscout-client-java/src/OSMScoutClient.cpp:811:      if (!osmscout::IsOpenableDatabaseDirectory(path)) {
```

The first is the validating entry point (it decides), the second is the bridge's report (it only logs).
No call site inside `RegisterAll()`. `OSMScoutClient.java` is untouched.

## 2. Unit tests

`Tests/src/DatabaseOpenTest.cpp` gains a self-removing `TempDirectory` helper (it replaces its
permissions with permissive ones before `remove_all`, so a case that made the tree unsearchable cannot
leave it behind), an `UnsearchableDirectory` scope guard (restores permissive access on the way out, so a
failed assertion cannot leave an unremovable directory), and `WriteFile()`. Nine cases were added:

- `An existing directory is openable, with or without a map in it` - empty, then with a `types.dat`.
- `A missing path is not openable`.
- `A regular file is not openable`.
- `A path below a regular file is not openable`.
- `A directory that cannot be searched is not openable` - `REQUIRE_NOTHROW` plus a false answer; skipped
  on Windows and when running as root.
- `RegisterOpenable registers an existing directory once` - set, size and set-change count; a second call
  is still a success without a duplicate.
- `RegisterOpenable leaves the set untouched for a missing path` - the set and the set-change count are
  unchanged.
- `RegisterOpenable rejects a regular file` - size 0, empty set, set-change count 0 (the case fails if the
  entry point registers before it validates).
- `RegisterAll still registers a path that is not a directory` - the predicate is asked first and answers
  false, then `RegisterAll` reports the disposition `true`, adds the path, and counts one set change.

```
cd build && ctest -R DatabaseOpenTest -V
  3: All tests passed (232 assertions in 19 test cases)
```

## 3. Java bridge

`libosmscout-client-java/src/OSMScoutClient.cpp`:

- `openDatabase(String)` registers through `data->knownPaths.RegisterOpenable(fsPath)`. On rejection it
  logs `[JNI] openDatabase: not an existing map database directory: <name>` and returns `JNI_FALSE`
  without calling `OnDatabaseListChanged`, so no set change is requested and every open database stays
  open. The report carries `fsPath.filename()` only.
- `openDatabases(String[])` is otherwise unchanged. It now asks the predicate for each path and logs
  `[JNI] openDatabases: not an existing map database directory: <name>` (again `filename()` only) before
  `RegisterAll(paths)`; the registration, the index-aligned boolean array and the single
  `OnDatabaseListChanged` are decided by `RegisterAll` alone. The batch therefore keeps reporting `true`
  for such a path.

No JNI signature changed, so no Java source file was edited.

```
bash scripts/check-jni-signatures.sh
  JNI signatures match: 55 native declarations checked, 6 dead JNI functions reported above.
  (exit code 0)
```

The 6 dead JNI functions are the pre-existing warnings of master. No Java declaration and no JNI
signature changed in this diff, so that list cannot differ from master's; the CTest case
`JniSignatureParityTest` passed in the full suite below as well.

## 4. Spec/code tension: the batch requirement

The reference implementation's `openDatabases()` does call the predicate, to log one warning per path that
is not an existing directory, while still registering the path. The requirement as first written ("The
batch registration does not inspect the filesystem") would have been false for that code. Reworded so that
code and spec agree on which part is binding:

- `specs/database-path-registry/spec.md`: the requirement is now "The batch registration never lets the
  filesystem decide". It states that the validating entry point is the only registration entry point that
  can refuse a path, that the predicate is the registry's only path check, that the list entry point
  registers whatever the filesystem says about a path, and that a report about a path of the list SHALL NOT
  change what the list registers. The new scenario `A report about a path of the batch changes nothing`
  pins exactly that, and the registry test case `RegisterAll still registers a path that is not a
  directory` implements it.
- `specs/client-java-database-paths/spec.md`: the requirement now says the batch call registers whatever
  the filesystem says about a path and reports a path that is not a directory "without changing its entry";
  the tolerance scenario gained the matching THEN line.
- `proposal.md` (What Changes, batch bullet) and `design.md` (D3) were reworded the same way.

So the rule is: the check may produce a report only. Registration, the per-input disposition and the
set-change count are decided by `RegisterAll()` and never by the filesystem. `openDatabase` is the only
entry point that validates and can refuse.

## 5. Builds, suite and validation

```
cmake --build build --target DatabaseOpenTest osmscout_client_java
  -> 30/30 targets built, no error and no warning from the touched files
cd build && ctest -R DatabaseOpenTest --output-on-failure
  -> 1/1 passed
meson compile -C build-meson DatabaseOpenTest osmscout_client_java libosmscoutclientjava
  -> 24/24 targets built, no error
meson test -C build-meson 'Check database path registration'
  -> 1/1 libosmscout:Check database path registration OK, 0 failures
cd build && xvfb-run -a ctest -j 2 --exclude-regex "PerformanceTest"
  -> 106/106 tests passed
openspec validate client-database-path-validation --strict
  -> Change 'client-database-path-validation' is valid
git diff --name-only origin/master
  -> libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h
     libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp
     libosmscout-client-java/src/OSMScoutClient.cpp
     Tests/src/DatabaseOpenTest.cpp
```

## Deviations from the reference implementation

- The reference's added test set had no case for "the batch still registers what it was handed"; one was
  added (`RegisterAll still registers a path that is not a directory`) because the reworded requirement and
  task 2.4 ask for it.
- The reference's comments and log lines tag spec capabilities `native-database-open` and
  `auto-diagnostics`, which do not exist in this repository. They were rewritten against this change's
  capabilities (`database-path-registry`, `client-java-database-paths`) and the "name only" rule was kept
  in the wording.
- The predicate's header documentation in the reference says it "is deliberately not used by the batch
  entry"; that is false for the bridge report, so the text now says that only `RegisterOpenable()` acts on
  the answer and that a report about a batch path changes nothing.

## Not run

- The JavaScout JUnit5 suite: no Java source or JNI signature changed, and the native path has no JUnit
  test of its own; the batch behaviour is pinned by the C++ registry test.
- `PerformanceTest` (excluded as CI does) and the sanitizer build.
- `scripts/format-check.sh` was not used as a gate: with the local `Uncrustify-0.83.0_f` it reports nearly
  every tracked file of the tree as unformatted, including files this change does not touch
  (`libosmscout/src/osmscout/util/*.cpp`), so it cannot distinguish this diff, and no CI workflow runs it.
  The added lines follow the formatting of the surrounding code and of the reference implementation.
