# Proposal

## Why

A deployment that ran unattended produced `nordrhein-westfalen: downloaded source failed
verification, discarding it` with no source that was actually corrupt, and the same can
happen to any import whose source changes while a source file from an earlier check is
still in the work area.

The regeneration script keeps the source of a failed import in the work area so a later
attempt does not download it again, and it resumes an interrupted download instead of
starting over. Both are decided from the *content* of the kept file: the script resumes
whenever the file does not match the published hash. That condition does not distinguish
the two cases such a file can be:

- a partial download of the source that is being fetched now - resuming is right, and it
  is the whole point of keeping it, and
- the complete source of an *earlier* check, left behind by a failed import - resuming
  appends the new source to the old one, so the result is a mixture of two versions of
  the extract.

The second case silently produces a file that fails verification. The run reports a
corrupt download that was never corrupt, discards the file, and downloads from scratch -
so the deployment recovers, but only after a full download it did not need and a log line
that points at the network instead of at the script. On a source that is re-published
daily, and with a `refresh` window of days, the two conditions meet regularly.

## What Changes

- The work area records which source a kept file belongs to, beside the file, so a later
  run can tell a partial of the source being fetched from the source of an earlier check.
- A kept file that belongs to another source is discarded before the transfer starts, so
  nothing of the earlier version can end up in the new download; a kept partial of the
  same source is resumed exactly as before.
- The record is written before a transfer starts, because the run that leaves a partial
  behind is the run that gets killed - that is the case the record has to cover.
- The failure-recovery check gains a case that reproduces the mixture (a kept complete
  source of another version) and a case that pins the resume of a partial of the same
  source, so the distinction is a tested contract and not a comment.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `regen-script`: the failure-handling and interrupted-work requirements gain the
  distinction between a partial of the current source (resumed) and a source kept from
  another check (discarded before the transfer), and the corresponding scenarios.

## Impact

- `scripts/mapgen/mapgen.sh` - the download path (`download_source`, around line 245) and
  the per-import download decision (`process_import`, around line 360): a record beside the
  kept source, the discard of a foreign one, and the removal of the record together with
  the file.
- `scripts/mapgen/recovery-check-test.sh` - two new cases in the failure-recovery check
  that run the script against a local source and a stub import tool, inside the mapgen
  image (CI smoke check) and outside it.
- `Documentation/MapRepository.md` - the failure-handling section states which kept source
  may be resumed and which is discarded.
- `openspec/specs/regen-script/spec.md` - the capability spec, through this change's spec
  delta.
- No change to the container image, the configuration files, the repository layout, or the
  client-side surface.
