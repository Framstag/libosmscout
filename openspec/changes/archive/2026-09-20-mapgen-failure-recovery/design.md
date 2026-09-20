# Design

## Context

See `proposal.md` - Why. The facts, from `scripts/mapgen/mapgen.sh`:

| line | today |
|---|---|
| 206 | `write_check_state` runs before the download and the import, so a failed pass is "not due" for `refresh` days |
| 199 | the hash fetch (`curl "$url.md5"`) has no timeout |
| 222 | `curl -fsSL "$url" -o "$pbf"` - no timeout, no retry, no resume, and no removal on failure |
| 231 | a source that fails verification is reported but kept on disk |
| 236-238 | the import output directory is deleted at the start of the *next* import, so one copy stays between passes |
| 267-271 | the served database is moved to `staging/<id>/v<version>.old` and the new one into place; a run killed in between leaves the served tree without that database |
| 222-273 | a successful import leaves the downloaded source in the work area for good |
| 327-332 | the main loop continues after a failing import and exits non-zero at the end - that part is already right |

## Decisions

### 1. The check state records a completed decision, not an attempt

`write_check_state` moves from before the download to the two paths that finish the decision for an import:
the "source unchanged" branch and the point after a successful placement. Every failure path leaves the state
as it was, which makes the import due on the next run and lets the existing refresh gate keep doing what it is
for - throttling checks of an *unchanged* source - instead of punishing a failure.

- Alternative - record a separate "attempt failed, retry in N minutes" state: a second clock to reason about,
  and it can starve an import that keeps failing slowly.
- Alternative - delete the check file on failure: equivalent in effect, but it would also lose the previously
  recorded hash, which the "unchanged" comparison uses as a cheap path.
- Rationale: one rule - the state describes what was decided, not what was tried - and the retry cadence is the
  schedule's, which is already configurable and already what the operator set.

### 2. Retry the download inside the run; never retry the import inside the run

A download is cheap and transient failures are common, so it is attempted up to a bounded number of times
inside the same run. An import is expensive (minutes to an hour) and its failures are usually deterministic -
bad data, a missing type module, a full disk - so retrying it inside the run would burn the schedule's time for
nothing; staying due brings it back on the next occurrence, which is also when a human would have looked.

### 3. Timeouts, resume and verification in that order

The hash fetch gets a short total timeout (it is a few dozen bytes). The source download gets a connect timeout
and an *idle* timeout (`--speed-limit`/`--speed-time`) rather than a total one, because a total timeout would
cut off a legitimately slow 912 MB transfer. A partial file is resumed on the next attempt (`-C -`); if the
service cannot resume (curl's range error), the partial file is removed and the next attempt starts from zero,
so an unsupported range never becomes a permanent failure. A file that fails verification is removed, because
resuming from an unverified partial is exactly how corrupt data would enter an import.

### 4. Interrupted replacement is rolled back by the next run

At the start of a pass, for each import, a leftover `staging/<id>/v<version>.old` is moved back into the served
tree when the target is missing (the run died between the two moves), and otherwise deleted (the replacement
completed and the copy is stale). Leftover `.new` directories are removed, since they are rebuilt anyway.

- Alternative - make the replacement a single atomic step: a directory swap has no portable atomic form; a
  symlink per slot would, but that changes the served layout the web server and the client contract depend on.
- Alternative - leave it to the next successful import: that is a full re-download and re-import, and until then
  the served tree is missing a database that the region index still advertises.
- Rationale: two lines of recovery at the start of a pass turn a gap in the served tree into a self-healing
  condition.

### 5. A successful run cleans up after itself

After the database is placed, the downloaded source and the import output directory are removed. A failed run
keeps the source so a later attempt can resume, and because the per-import file names are fixed, later runs
overwrite rather than accumulate.

## Risks / Trade-offs

- Removing the downloaded source after a success means a re-import of the same source (a type-config change, a
  manual re-run) downloads it again → the alternative is keeping up to a gigabyte per import in the work area
  forever, which is worse for the unattended case; the work area stays small and the source is one download.
- Bounded retries lengthen a run that is going to fail anyway → the retries are few and spaced, and the
  alternative is a week of waiting.
- The idle timeout can abort a transfer whose server stalls briefly → the attempt is retried, and the partial
  file is resumed, so the cost is the wait, not the data.
- A rollback restores a database the operator may have wanted replaced → the next successful import replaces it
  again, and the generation record shows what happened.

## Migration Plan

Nothing to migrate for a healthy deployment: the first successful pass after the update removes what earlier
passes had left behind (the source and the import output of the run it performs), and a deployment that has
been failing starts recovering on the next occurrence instead of waiting for the refresh window.
