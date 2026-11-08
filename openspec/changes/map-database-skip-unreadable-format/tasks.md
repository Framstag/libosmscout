## 1. Scan reads the data-format version (spec: map-database-discovery)

- [x] 1.1 In `libosmscout-client/src/osmscoutclient/MapManager.cpp`, add a file-local
  `ReadDatabaseFileFormatVersion` that returns the directory's version via
  `TypeConfig::GetDatabaseFileFormatVersion`, and returns no value (logging at debug) when that read
  throws, so an unreadable version decides nothing. Verify: the helper is in an anonymous namespace
  and adds no public API.
- [x] 1.2 In the scan loop, before constructing `MapDirectory` / calling `IsValid()`, read the
  candidate's version and, when it has a value and differs from `FILE_FORMAT_VERSION`, count the
  directory invalid, log one warning naming the directory and both versions, and `continue`. Verify:
  a version-less or unreadable directory reaches the previous code path unchanged.

## 2. Host tests (spec: map-database-discovery)

- [x] 2.1 In `Tests/src/MapManagerTest.cpp`, add `CreateDatabaseWithFormatVersion`, which fabricates
  the mandatory files and writes the first `byteCount` bytes of a chosen little-endian version into
  `types.dat`; a `byteCount` below 4 makes the version unreadable. Verify: the helper needs no real
  database and no import.
- [x] 2.2 Add the case "A scan skips a database whose data format this build does not read": a
  version-less database stays published while `FILE_FORMAT_VERSION-1` and `FILE_FORMAT_VERSION+1` are
  skipped, and the published set holds only the version-less database.
- [x] 2.3 Add the case "A database whose data format version cannot be read stays published": a
  truncated `types.dat` is published beside a valid database. Verify: both cases exercise the two
  requirements and fail against the pre-change scan.

## 3. Build and verification (spec: map-database-discovery)

- [x] 3.1 Reconfigure `build/` with the tree's existing options (`cmake -S . -B build`) and build the
  exact target: `cmake --build build --target MapManagerTest`. Verify: compiles without error.
- [x] 3.2 Run `ctest -R MapManagerTest --output-on-failure` from `build/`. Verify: passes, and the two
  new cases are present via `build/Tests/MapManagerTest --list-tests`.

## 4. Spec and change hygiene

- [x] 4.1 Run `openspec validate map-database-skip-unreadable-format --strict` and verify it passes.
- [x] 4.2 Verify the spec delta introduces `map-database-discovery` and touches no existing capability.

## 5. Verification evidence

See `verification.md` for the exact commands and their results.
