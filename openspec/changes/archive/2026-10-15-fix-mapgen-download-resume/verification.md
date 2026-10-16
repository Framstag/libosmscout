# Verification: fix-mapgen-download-resume

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- Host: Linux, `bash` 5, `curl`, `jq`, `md5sum`, `shellcheck` available; **no Docker daemon**
  (`docker version` -> `failed to connect to the docker API at unix:///var/run/docker.sock`),
  so the CI smoke path could only be run outside the image.
- The check runs against a fixture tree, with a local source (`file://`) and a stub import
  tool, exactly as CI runs it inside the image:
  ```
  MAPGEN_CHECK_DIR=/tmp/mapgen-check MAPGEN_WORK_DIR=/tmp/mapgen-check \
    MAPGEN_SCRIPT=$PWD/scripts/mapgen/mapgen.sh \
    bash scripts/mapgen/recovery-check-test.sh
  ```
  `MAPGEN_WORK_DIR` is set to the fixture root only because the default (`/work`) does not
  exist on this machine; the script under test is given the work area explicitly
  (`MAPGEN_WORK_DIR` in `run_pass`).
- The files this change touches are shell and Markdown only:
  `git diff --name-only` -> `Documentation/MapRepository.md`, `scripts/mapgen/mapgen.sh`,
  `scripts/mapgen/recovery-check-test.sh`. No C++ file is modified, so the C++ build and the
  CTest/Meson suites carry no new code to verify; they were not rebuilt.

## 1. The resume decision

### 1.1 Record beside the kept source

`scripts/mapgen/mapgen.sh` gains four helpers next to the download section:
`source_record` (the path of the record, `<pbf>.source-md5`), `record_source`,
`source_belongs_to` (record present, file present, record equals the published hash) and
`discard_source` (removes file and record together). `source_belongs_to` requires the file,
so a record left without a file can never make a resume happen.

```
bash -n scripts/mapgen/mapgen.sh   -> ok (no output)
```

### 1.2 The record is written before the transfer

`download_source` now takes the published hash as its third argument and calls
`record_source` before the attempt loop, so a run killed mid-download leaves a partial that
the next run can both identify and resume. The status-33 branch keeps `rm -f "$pbf"` and the
record: the record still names the source, and an attempt that leaves a partial behind after
the restart stays resumable.

### 1.3 A foreign kept source is discarded

`process_import` keeps the content-based reuse branch and, in the fall-through, discards a
kept file whose record does not name the published hash - logging
`<id>: discarding a source kept from another check` - before calling `download_source`. The
message is what makes the condition visible to an operator instead of leaving a misleading
verification failure behind.

### 1.4 One removal path

The three sites that throw a source away - a successful placement, a verification failure,
and the foreign-source discard - call `discard_source`. `grep -n 'rm -f "$pbf"'` reports the
remaining one in the status-33 branch, which deliberately keeps the record.

## 2. Test coverage

### 2.1 and 2.2 The two new cases

`scripts/mapgen/recovery-check-test.sh` gained two cases after the verification case:

- `a source kept from an earlier check is not resumed onto`: the work area holds 4096 random
  bytes as "the complete source of an earlier version", the fixture source changed since.
  Asserted: the discard is logged, no verification failure is reported, the database is
  placed, and neither the source nor its record is left behind.
- `a partial of the same source is resumed`: the work area holds the first 1024 bytes of the
  current source plus a record naming the published hash. Asserted: no discard is logged and
  the database is still imported and placed.

The download-failure case needed a fixture of its own (`rm -rf "${CHECK_DIR}"; build_fixture`)
because it previously inherited its precondition - a due import - from the case before it,
which now performs a successful pass that records the check state. Nothing else about that
case changed.

```
MAPGEN_CHECK_DIR=/tmp/mapgen-check MAPGEN_WORK_DIR=/tmp/mapgen-check \
  MAPGEN_SCRIPT=$PWD/scripts/mapgen/mapgen.sh bash scripts/mapgen/recovery-check-test.sh
-> recovery check OK (8 cases)
```

### 2.3 The new case detects the defect

The same check run against the pre-change script (`git show HEAD:scripts/mapgen/mapgen.sh`,
with `MAPGEN_BASEMAP_SCRIPT` pointing at the in-tree basemap script so the copy can run):

```
case 5: a source kept from an earlier check is not resumed onto
  FAIL: a source kept from another check must be discarded
  FAIL: a source kept from another check must not be resumed onto
  FAIL: the import must still place its database
    | [mapgen] berlin: source changed, downloading
    | [mapgen] ERROR: berlin: downloaded source failed verification, discarding it
recovery check FAILED: 3 case(s)
```

That is the reported production incident reproduced in miniature - the same
`downloaded source failed verification` line, produced by a kept file that was never
corrupt. Case 6 (`a partial of the same source is resumed`) passes with the pre-change script
as well, which is the intended behaviour that must not regress.

### 2.4 All cases

All eight cases pass with the change; the six pre-existing cases are unchanged apart from the
fixture reset noted above.

## 3. Documentation

`Documentation/MapRepository.md`, failure-handling section: the download bullet keeps its
tunables and gains a bullet stating that a kept source is resumed only while it belongs to
the source being fetched, that the work area records the published hash beside the file
before the transfer starts, and what happens to a file whose record names another source.
The neighbouring bullet (reuse on a matching hash, cleanup after a placement) is unchanged.

## 4. Integration and bookkeeping

- `bash -n` passes for both shell files.
- `shellcheck -s bash` reports only pre-existing findings (mapgen.sh lines 126/127, 496,
  503, 505; recovery-check-test.sh line 45), none in the added lines.
- The CI smoke path (`.github/workflows/mapgen_image.yml`, step "Recovery check" and the
  script's use as the failure-recovery harness inside the image) was **not** run: no Docker
  daemon on this machine and no image built here. The check itself does not depend on the
  image (it needs bash, curl, jq, md5sum and a local source), which is why running it outside
  the image covers the same code path; the image only changes `PATH` and `MAPGEN_WORK_DIR`.
- `AGENTS.md` needs no update: no structural, build or tooling change.
- `TODO.md` gets no entry: the defect is this change's, and the two observations made while
  applying it (the test case's implicit precondition, the stale doc bullet) belong to the
  files this change edits.
- `openspec/specs/regen-script/spec.md` is updated through this change's spec delta
  (`specs/regen-script/spec.md`); the capability spec itself is rewritten on archive.

## Residual risks

- The record is a second file beside the source. A work area modified by hand could leave a
  record that does not match its file; the resume check requires both and the content check
  runs first, so the worst case is a full re-download.
- Deployments that already have a source in the work area at the moment of the upgrade have
  no record for it, so the first pass after the upgrade discards it and downloads again -
  once, and only for sources that a previous run left behind.
