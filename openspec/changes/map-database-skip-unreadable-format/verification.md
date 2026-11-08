# Verification evidence

Revision: branch `map-database-skip-unreadable-format` off `origin/master`
(`9dedb7385`, "Merge pull request #1912 ...").

## 1. Diff

- `git diff --stat` — exactly the two source files of the proposal's Impact section:

  ```
   Tests/src/MapManagerTest.cpp          | 70 ++++++++++++++++++++++
   .../src/osmscoutclient/MapManager.cpp | 37 ++++++++++++
   2 files changed, 107 insertions(+)
  ```

  No database, map, style-sheet, type-definition or file-format-version file is in the diff, so no
  `FileFormatVersion.md` bump applies. The change only reads a version; it writes none.

## 2. Build

- `cmake -S . -B build` — reconfigured the existing `build/` with its cached options (no option was
  invented); the cached CMake path was stale, so this regenerated `build.ninja`. Reported
  `Configuring done` / `Generating done`.
- `cmake --build build --target MapManagerTest` — built only that target. Result: compiled
  `OSMScoutClient` and the test unity unit, linked `Tests/MapManagerTest-1.1.1`, exit 0, no error
  and no warning from the touched files.

## 3. Test

- `ctest -R MapManagerTest --output-on-failure` (from `build/`):

  ```
  1/1 Test #26: MapManagerTest ...................   Passed    0.23 sec
  100% tests passed out of 1
  ```

- `build/Tests/MapManagerTest --list-tests` — 9 test cases, including the two new ones:
  `A scan skips a database whose data format this build does not read` and
  `A database whose data format version cannot be read stays published`.
- `build/Tests/MapManagerTest "A scan skips a database whose data format this build does not read"` —
  run directly to read the scan's own output. It shows the skip and the summary:

  ```
  Lookup directory .../db-unreadable-version scan complete: 3 candidates, 1 valid, 2 invalid
  Total installed maps found: 1
    skipping map directory whose data format version is 28, this build reads 27: .../db-newer-version
    skipping map directory whose data format version is 26, this build reads 27: .../db-older-version
  All tests passed (4 assertions in 1 test case)
  ```

  So the warning names the directory and both versions, and the skipped directories count as
  invalid.

## 4. The new check has teeth

The pre-change scan was run to confirm the new case actually pins the behaviour:

- `git stash push -- libosmscout-client/src/osmscoutclient/MapManager.cpp` (test kept),
  `cmake --build build --target MapManagerTest`, `ctest -R MapManagerTest --output-on-failure`:
  `A scan skips a database whose data format this build does not read` failed with
  `REQUIRE( manager->GetDatabaseDirectories().size()==1 )` / `with expansion: 3 == 1` — the
  pre-change scan published all three directories, including the two with a version this build does
  not read. (`A database whose data format version cannot be read stays published` passed both
  before and after, which is its point: it pins that the change does not start skipping directories
  it cannot judge.)
- `git stash pop`, rebuild, `ctest -R MapManagerTest --output-on-failure`: passed again.

## 5. Spec validation

- `openspec validate map-database-skip-unreadable-format --strict` —
  `Change 'map-database-skip-unreadable-format' is valid`.

## 6. What is not verified here, and why

- No end-to-end check against a real map database was run. The databases under `maps/` are format
  v26 while the library expects v27, so they cannot be opened, and importing a v27 database is out
  of scope for this change. The `MapManagerTest` stays a host test that fabricates its own directory
  tree and `types.dat`, which is exactly how the scan decision is pinned.
- The reader's own refusal of a database it cannot read is unchanged and is not re-tested; the change
  deliberately leaves an unreadable-version directory to the reader.
- `scripts/format-check.sh` is red repo-wide on this machine (including pristine master), so it is
  not used as a gate; the added lines follow the style of the code around them.
