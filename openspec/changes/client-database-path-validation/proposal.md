# Proposal

## Why

The single-directory open answers success even when the path it was given does not exist or is a regular
file. The path enters the registered set, the database thread is asked to process a set change that cannot
open anything, and the caller is told nothing — so a mistyped or vanished map directory looks registered
until the user notices the missing map.

## What Changes

- The client library SHALL offer a registration entry point that accepts a path only when the path exists
  and is a directory, and answers whether the path is part of the registered set afterwards.
- The answer SHALL be decided without raising: a missing path, a regular file and a permission failure
  SHALL all be answered as "not registered".
- A rejected path SHALL enter no set and SHALL count no set change, so nothing is published to the database
  thread and every database already open stays open.
- The single-directory Java call SHALL reject such a path and report failure to the caller.
- The batch registration SHALL keep registering the paths it was handed, whatever the filesystem says
  about them, so a directory that disappears between a caller's scan and the call cannot fail the batch.
  A path of that batch that is not a directory SHALL be reported, and that report SHALL NOT change what
  the batch registers, the result it reports per input or the number of set changes it counts.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `database-path-registry`: gains a registration entry point that validates the path, and states that the
  batch entry point deliberately does not.
- `client-java-database-paths`: the single-directory call no longer registers a path that is not a
  directory; the batch call keeps its tolerance and gains a report for such a path.

## Impact

Affected files and modules:

- `libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h` — a documented predicate "is this an
  openable database directory?" and the validating registration entry point, both public API.
- `libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp` — implementation, decided outside the
  registry lock.
- `Tests/src/DatabaseOpenTest.cpp` — extends the existing registry test with the predicate and the
  validating entry point, using a real temporary directory and real files.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the single-directory call uses the validating entry
  point and reports a rejection; the batch call keeps registering and reports a path that is not a
  directory by name only.

No database, map or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature changes: the single-directory call already returns a boolean, and the batch
call already returns an index-aligned boolean array. The batch result for a non-directory path is
unchanged (it is registered), so no existing caller's contract changes.
