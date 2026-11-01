## Context

See `proposal.md` — Why. The relevant current state:

- `MapManager::LookupDatabases` walks every configured lookup directory recursively, and for each
  regular file named `types.dat` it constructs a `MapDirectory` around the file's parent directory
  and asks `MapDirectory::IsValid()`. A directory that passes is published; everything else is
  counted invalid and skipped.
- `MapDirectory::IsValid()` checks that the mandatory database files exist and are non-empty. It does
  not read the data-format version, so a directory with a `types.dat` of a version this build does
  not read passes the scan and is registered.
- `TypeConfig::GetDatabaseFileFormatVersion(directory)` reads that version from the directory's
  `types.dat` — the only database file with a version header — and throws (`IOException`, and any
  other failure of the underlying file read) when the file is missing, shorter than the version
  field, or unreadable. `FILE_FORMAT_VERSION` is the version this build reads.
- The databases under `maps/` are a different format version than the library expects on this
  machine, so an end-to-end check against them cannot be run; the test fabricates its own tree.

## Goals / Non-Goals

**Goals:**

- Keep a directory whose data format this build does not read out of the published database set.
- Make the skip visible with one log line that names the directory and both versions.
- Never let the new check refuse a directory the scan cannot judge — an unreadable version is not a
  mismatch, so the reader keeps deciding it.
- Keep the scan's existing bookkeeping: a skipped directory is invalid, like any other the scan does
  not publish.

**Non-Goals:**

- Changing `MapDirectory::IsValid()`, the reader, or how an unreadable database is reported when it
  is opened.
- Changing which directories count as candidates, or the recursive walk, or the publication signal.
- Reformatting, migrating, or otherwise touching the databases the scan reads.

## Decisions

**D1 — The scan reads the version, not the reader's later refusal.**
The version is read at scan time from the candidate's own `types.dat`, before the directory is
admitted. Alternatives:
- *Leave it to the reader, as today*: the database is registered first and refused later, so a
  consumer sees an open-time error and a set that already contains the unusable entry.
- *Let `MapDirectory::IsValid()` reject it*: `IsValid` is shared with other callers and has no
  format-version contract; widening it would change what "valid" means beyond the scan.
Chosen because the scan is where the set is decided, and the version is exactly the fact the scan
needs.

**D2 — An unreadable version means "leave it to the reader", not "skip".**
The helper returns no value when `GetDatabaseFileFormatVersion` throws; the scan then admits the
directory exactly as before. Alternatives:
- *Treat a missing or unreadable version as a mismatch*: a directory whose version cannot be read is
  not a directory this build cannot read — the reader may still open it, and the scan would start
  refusing databases it cannot judge.
- *Warn on the unreadable case too*: the reader reports its own error when it refuses such a
  directory, so a second warning from the scan would duplicate it.
Chosen because the reader stays the authority on any directory the scan cannot judge, which is the
invariant the change must not break.

**D3 — The skip is one warning and one invalid count.**
A skipped directory logs `Warn`, not `Info`, because it is a user-visible omission rather than a
routine scan note; it increments the same `invalidCount` as an invalid directory, so the existing
per-directory summary line stays correct. Alternatives:
- *Log at `Info`*: the skip would be lost among the per-directory scan notes, and the point of the
  change is that the omission is named.
- *A separate counter*: would need a new summary line and a wider diff for no reader-facing gain.
Chosen because it reuses the scan's existing structure and names what it drops.

**D4 — The version read is a file-local helper.**
`ReadDatabaseFileFormatVersion` sits in an anonymous namespace in `MapManager.cpp`; it wraps
`TypeConfig::GetDatabaseFileFormatVersion` and turns its exception into `std::nullopt`. It is not a
public API. Alternatives:
- *Inline the try/catch in the loop*: hides the fail-open rule inside the scan body and mixes it with
  the bookkeeping.
- *Add a non-throwing overload to `TypeConfig`*: a core API change for a client-local concern.
Chosen because the rule — an unreadable version decides nothing — is the part worth naming, and it
is host-testable through the scan.

## Risks / Trade-offs

- *A directory whose `types.dat` happens to begin with the version bytes but is otherwise unreadable*
  → the version can be read, so the scan judges it; this is exactly the case the scan is meant to
  handle, and a version that matches is admitted as before.
- *A directory whose version cannot be read is still published and still refused by the reader later*
  → unchanged from today, and deliberate: the scan does not guess. The reader's error is the
  authority on that directory.
- *The version read is an extra file open per candidate* → one small read per directory that holds a
  `types.dat`, on the scan's worker thread; negligible beside the recursive walk that found it.
- *A directory with several nested `types.dat` files* → each is judged on its own, as each candidate
  already is.

## Verification

- The scan decision is host-tested in `Tests/src/MapManagerTest.cpp` without a real database: the
  test fabricates a directory of the mandatory files whose `types.dat` carries a chosen little-endian
  version (the only database file with a version header), so a version equal to, below and above
  `FILE_FORMAT_VERSION` can be produced, and a below-field-length `types.dat` stands in for the
  unreadable case. No map database is opened, which is required on this machine because the installed
  databases are a different format version and importing one is out of scope.
- The reader's own refusal of an unreadable database is unchanged and is not re-tested here.

## Open Questions

- Whether the skip should be logged once per directory or aggregated into one line per scan:
  deferrable, one line per directory matches the existing per-directory scan output.
