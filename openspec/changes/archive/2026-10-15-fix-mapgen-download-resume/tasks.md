# Tasks

## 1. The resume decision (spec: regen-script - "Failure handling", "Recovery from interrupted work")

- [x] 1.1 In `scripts/mapgen/mapgen.sh`, add the work-area record of a kept source beside it: helpers for the record path, for writing it, for asking whether a file belongs to a given published hash, and for removing file and record together; verify with `bash -n` that the script parses.
- [x] 1.2 In `download_source` (line ~245) write the record with the published hash before the attempt loop starts, and take the published hash as a parameter; keep the `curl -C -` resume and the status-33 restart; verify the script still runs a pass against the fixture source of `scripts/mapgen/recovery-check-test.sh`.
- [x] 1.3 In `process_import` (line ~360) discard a kept `$pbf` that does not belong to the published hash before calling `download_source`, logging the import id and the reason; verify a pass on a fixture with a kept foreign file logs the discard and imports successfully.
- [x] 1.4 Route every removal of `$pbf` (successful placement at line ~435, verification failure at line ~381, foreign-source discard) through the helper that removes the record too; verify no `rm -f "$pbf"` for those three sites remains.

## 2. Test coverage (spec: regen-script - "Failure handling" - "A source kept from an earlier check is discarded", "A partial download is resumed")

- [x] 2.1 Extend `scripts/mapgen/recovery-check-test.sh` with a case that reproduces the mixture: a complete source of an earlier version is in the work area, the source changed since, and the pass must discard the kept file, not report a verification failure, and still place its database.
- [x] 2.2 Add the counter-case: a partial of the *current* source, with the record naming it, must not be discarded and must still be imported and placed.
- [x] 2.3 Show the new case detects the defect: run the check with the pre-fix script (for example `git stash` of the `mapgen.sh` changes, or the version from `HEAD`) and record the failure it produces in `verification.md`.
- [x] 2.4 Run the whole check in a fixture tree (`MAPGEN_CHECK_DIR`, `MAPGEN_SCRIPT`) and report the case count; verify every pre-existing case still passes.

## 3. Documentation (spec: regen-script - "Failure handling")

- [x] 3.1 Update the failure-handling section of `Documentation/MapRepository.md` (line ~444) so it states which kept source may be resumed and that a source kept from another check is discarded before the transfer, so the documented behaviour matches the script.

## 4. Integration and bookkeeping

- [x] 4.1 Verify the change touches shell only: run `bash -n scripts/mapgen/mapgen.sh` and `bash -n scripts/mapgen/recovery-check-test.sh`, and confirm the C++ build and the test suite are unaffected (no C++ file is modified; state this in `verification.md` instead of a rebuild claim that cannot be made here).
- [x] 4.2 Check the container smoke path the check runs in on CI (`.github/workflows/mapgen_image.yml`, the `recovery-check-test.sh` step) and note in `verification.md` whether it was run inside the image on this machine or only outside it.
- [x] 4.3 Write `verification.md` with the commands and their output, mark the tasks complete, and confirm no `TODO.md` entry is needed (the defect is this change's, not a pre-existing one found beside it).
