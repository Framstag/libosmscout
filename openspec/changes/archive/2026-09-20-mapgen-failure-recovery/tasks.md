# Tasks

## 1. The check state records a completed decision

- [x] 1.1 Move `write_check_state` in `scripts/mapgen/mapgen.sh` from before the download to the paths that complete the decision for an import: the "source unchanged" branch and the point after the database is placed. Verify: a stub-based run whose import fails leaves the state file unchanged and a second run is due again; a run whose source is unchanged and a run that places a database both record the state
- [x] 1.2 Keep the refresh gate's existing behaviour for unchanged sources (spec `regen-script`: check-cycle state). Verify: with a state recorded and a source that has not changed, a second run reports "not due" inside the refresh window and "source unchanged" outside it, without downloading

## 2. Downloads recover

- [x] 2.1 Add timeouts to the hash fetch and to the source download (connect timeout, and an idle timeout rather than a total one), so a stalled transfer ends the attempt (spec `regen-script`: failure handling). Verify: with a source that delivers nothing, the attempt ends within the idle timeout instead of hanging, and the run reports it
- [x] 2.2 Retry the download within the same run, bounded, before the import counts as failed (spec `regen-script`: failure handling). Verify: with a source that fails on the first attempt and succeeds on the second, the run imports and places without a second pass; with a source that always fails, the run reports the import as failed after the configured number of attempts
- [x] 2.3 Resume a partial download on a later attempt, remove the partial file when the service cannot resume, and discard a download that fails verification (spec `regen-script`: source verification, failure handling). Verify: an interrupted download continues from its previous size on the next attempt; a source whose hash does not match leaves no source file behind and no import; a resumed download is verified before it is used

## 3. Interrupted work is recovered

- [x] 3.1 Recover an interrupted replacement at the start of a pass: a leftover `staging/<id>/v<version>.old` is moved back when the served target is missing, and removed otherwise; leftover `.new` directories are removed (spec `regen-script`: recovery from interrupted work). Verify: with `staging/<id>/v27.old` present and `public/<path>/v27` absent, a pass restores the served database; with both present, the leftover is removed
- [x] 3.2 Remove the downloaded source and the import output of an import after its database is placed, and keep the source when the import failed (spec `regen-script`: recovery from interrupted work). Verify: after a successful stub-based import the work area holds only the crontab, the lock and the per-import log; after a failed one the source remains for a resume, and a second failed run does not add another copy

## 4. Verification and documentation

- [x] 4.1 Add a smoke check to `.github/workflows/mapgen_image.yml` that runs the script inside the image with a stub import tool and a local source, covering: a failed import stays due, a failed download is retried and stays due, a successful import places the database, records the state and cleans the work area, and an interrupted replacement is restored (spec `regen-script`: all scenarios of the modified and added requirements). Verify: on the runner the check passes, and it fails when the check state is written before the work or the retry is removed
- [x] 4.2 Keep the existing smoke checks green (spec `regen-script`: failure handling). Verify: the config check, the single pass, the identity, the scheduled-mode and the type-configuration checks all still pass in the same run
- [x] 4.3 Update the failure-handling part of `Documentation/MapRepository.md`: what recovers by itself (retries, resume, discarded sources, rolled-back replacements, staying due) and what an operator still has to do, namely nothing but watch the exit status and the log. Verify: the section names each recovery and matches the behaviour the smoke check exercises
- [x] 4.4 Record what this leaves open in `TODO.md` if anything does (spec `regen-script`: recovery from interrupted work). Verify: the entry, if any, names the condition and what would close it

## 5. Verification

- [x] 5.1 Validate the change artifacts (spec `regen-script`). Verify: `openspec validate --change mapgen-failure-recovery --strict` passes
- [x] 5.2 Verify the whole path on a runner: the image builds, the recovery smoke check passes, and the other checks stay green (spec `regen-script`: all scenarios). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified locally with a stub import tool, a `file://` source and its md5 sidecar, no Docker and no real OSM
data. Six scenarios, all as specified:

| scenario | result |
|---|---|
| a failing import | the pass exits 1, records no check state, and a second run reports `checking for changes` (still due) |
| a successful import | exits 0, places `v27`, records the check with the source hash, writes the generation record with the right `generatedAt`, and leaves only the per-import log in the work area |
| a source that fails verification | reported and removed; no import; nothing left to resume from |
| a download that fails | attempted twice (the configured number), with the real curl status logged, then recorded as failed |
| a source left by a failed import | kept, and the next run reports `reusing the verified source from the work area` instead of downloading 900 MB again |
| an interrupted replacement | with a fresh check state the pass does nothing else and reports `restored europe/germany/berlin/v27 after an interrupted replacement`; the leftover is gone and the slot is served again |

The two bug fixes found this way were covered by the same runs: the download's curl status is now captured before
the conditional consumed it, and `generatedAt` is read before the work area is cleaned. `bash -n` passes on the
script, the workflow parses with all 21 step scripts passing `bash -n`, and `openspec validate --strict` passes
(tasks 1.1 to 4.4, 5.1).

### The recovery check's first runner runs

It failed twice, and both times the check was at fault, not the script.

**Piping the pass into `grep -q`.** `grep -q` leaves as soon as it matches and thereby closes the pipe, so the
pass died of SIGPIPE right after printing the line the check was looking for - no import, no placement, and the
next assertion (`the database was not placed`) failed. The local harness had captured the output into a
variable, which is why the same scenarios passed there. Reproduced side by side:

| form | result |
|---|---|
| `mapgen.sh \| grep -q "reusing the verified source"` | grep matched, the pass was killed, nothing placed |
| `out=$(mapgen.sh); echo "$out" \| grep -q ...` | the database was placed and the run reported it |

**Single quotes inside the container script.** The whole check was passed to the container as `sh -c '...'`,
and the `jq` filter inside it used single quotes, which ended the outer quoting early: the filter arrived as
`{lastCheckedAt:` and `jq` exited 3 with `syntax error, unexpected end of file (Unix shell quoting issues?)`.

Both are gone because the check is no longer a shell string assembled in the workflow: it is
`scripts/mapgen/recovery-check-test.sh`, which builds its own fixture in the writable work area and is run as a
file with bash (`docker run … --entrypoint bash <image> /check/recovery-check-test.sh` - bash rather than the
container's `sh`, because the script uses bash syntax). That also makes it runnable outside the image - the same
file was executed here against the real `mapgen.sh`, six cases, and passed - so what CI exercises is the artifact
that was tested locally rather than a variant of it.

### On the runner

Run `35492719226` (head `4562babc8`): the `verify` job passed every step, including
`Smoke test (recovery of failed runs)` inside the image, the compose job passed, and the publish job was skipped,
as a pull request should. With the local run of the same script (six cases against the real `mapgen.sh`), every
scenario of the modified and added requirements is verified - tasks 4.2 and 5.2 were the last, so this change is
complete.

Open, and to be observed on a runner: the image builds, the new recovery smoke check passes inside it, and the
existing checks stay green (tasks 4.2, 5.2).
