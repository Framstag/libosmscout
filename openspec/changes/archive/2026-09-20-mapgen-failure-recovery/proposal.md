# Proposal

## Why

The regeneration process is meant to run unattended: a container that wakes up on a schedule, imports what
changed and goes back to sleep, for months, without anyone watching it. Today a single failure stops it for
the whole refresh window, and a single stalled transfer stops it forever:

- the check state is written before the download and the import are attempted, so an aborted or failed pass
  is "not due" for the next `refresh` days - a manual `rm` is what actually repairs a deployment;
- the download has neither a timeout nor a retry and never resumes, so a stalled transfer blocks the pass
  while holding the exclusive lock - every later occurrence is skipped and nothing notices;
- an import that fails on a broken source is attempted again only after the refresh window, but a partial
  download that fails verification is kept and would be resumed from if a later attempt resumed;
- a pass killed between moving the served database aside and moving the new one in leaves the served tree
  without that database until a complete re-download and re-import succeeds;
- a successful import leaves the downloaded source and the import output in the work area, so a long-lived
  container keeps them for good.

## What Changes

- A failing import SHALL stay due, so that the next occurrence retries it without waiting out the refresh
  window. The check state SHALL record a completed decision - unchanged source, or imported and placed - not
  an attempt.
- A download SHALL be retried within the same pass, a bounded number of times, before the import is recorded
  as failed. An import itself SHALL NOT be retried within the pass: it is expensive and its failure is
  usually deterministic, and staying due is what brings it back.
- Downloads and hash fetches SHALL have timeouts, including an idle-transfer timeout, so that a stalled
  transfer ends the attempt instead of holding the exclusive lock indefinitely.
- A partial download SHALL be resumed by a later attempt, and a download that fails verification SHALL be
  discarded rather than kept or resumed from.
- An interrupted replacement SHALL be finished or rolled back by the next pass, so the served tree always
  holds either the previous database or the new one.
- A successful pass SHALL leave neither the downloaded source nor the import output behind, so the work area
  does not grow with the number of passes.
- Each pass SHALL report the outcome per import and SHALL keep exiting non-zero when any import failed.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `regen-script`: the check-cycle state requirement changes from "store the check time" to "store it when the
  check is completed", and the failure-handling requirement gains recovery: bounded retries, timeouts, resume
  and discard rules, interrupted replacements, and a bounded work area.

## Impact

Affected files and modules:

- `scripts/mapgen/mapgen.sh` - the check-state write moves to the completed paths, the download gains timeouts
  and retries and resumes, a failed verification discards the source, an interrupted replacement is recovered
  at the start of a pass, and a successful import removes its inputs from the work area.
- `.github/workflows/mapgen_image.yml` - a smoke check that exercises the failure paths inside the image with a
  stub import tool and a local source: a failed import stays due, a failed download stays due and is retried,
  a successful import places and cleans up and records the state, and an interrupted replacement is restored.
  The check itself is `scripts/mapgen/recovery-check-test.sh`, a script so that it can be run outside the image
  as well.
- `scripts/mapgen/recovery-check-test.sh` (new) - the check above: it builds its own fixture in the writable
  work area (a stub import tool, a source file with its hash sidecar) and asserts each recovery behaviour.
- `Documentation/MapRepository.md` - the failure-handling part states what recovers automatically and what an
  operator still has to do.
- `TODO.md` - anything this leaves open.
- openspec change `mapgen-failure-recovery` records the requirements.
- No database format, style sheet, import tool, build system or packaging change, and no new dependency.
