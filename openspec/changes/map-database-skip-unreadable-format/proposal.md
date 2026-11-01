## Why

`MapManager::LookupDatabases` registers every directory that holds a `types.dat`, even when that
database's data-format version is not one this build reads. `MapDirectory::IsValid()` checks only the
presence of the mandatory files, not the version. The unusable database therefore lands in the
database list first and is refused later, when the reader opens it — after the other databases have
already been registered. A consumer that iterates the list hits a database that never opens, with an
error that names the open rather than the scan, and one bad directory can spoil a set that is
otherwise fine.

## What Changes

- The scan SHALL read a candidate directory's database data-format version up front, from its
  `types.dat`, and SHALL leave the directory out of the published set when that version is present
  and is not the version this build reads.
- The scan SHALL log one warning naming the skipped directory and both versions, so the skip is
  visible instead of silent.
- A directory whose data-format version cannot be read — a missing, shorter-than-the-version-field,
  or unreadable `types.dat` — SHALL stay a candidate and SHALL still be offered to the reader. The
  reader stays the authority on such a directory.
- A skipped directory SHALL count as invalid in the scan's summary, like any other directory the
  scan does not publish.

## Capabilities

### New Capabilities

- `map-database-discovery`: the client's scan of the configured lookup directories — which
  directories are published as map databases, and how a directory whose data format this build does
  not read is handled.

### Modified Capabilities

- None.

## Impact

Affected files and modules:

- `libosmscout-client/src/osmscoutclient/MapManager.cpp` — `LookupDatabases` reads the candidate's
  data-format version before admitting it to the set, and a small file-local helper reads that
  version and reports an unreadable one as "no value".
- `Tests/src/MapManagerTest.cpp` — two host test cases fabricate directory trees with a
  `types.dat` whose first bytes carry a chosen version: one asserts that an older and a newer version
  are skipped while a version-less database stays published, the other that a truncated version is
  still published.

The scan uses the existing `TypeConfig::GetDatabaseFileFormatVersion`, which throws when the version
cannot be read and returns it otherwise; no new core API, no database or map file format change, and
no new dependency. `MapDirectory::IsValid()` and the reader are unchanged and stay the authority on
any directory this scan admits. No file-format version bump applies (`FileFormatVersion.md`): the
scan only reads the version, it does not write one.
