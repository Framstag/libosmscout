# Tasks

## 1. The two new scripts

- [x] 1.1 Add `scripts/mapgen/mapgen-pass.sh`: one pass under an exclusive lock in the work area, reporting and exiting successfully when the lock is held by another pass (spec `mapgen-container`: overlapping passes are skipped). Verify: two concurrent invocations show one pass running and the second reporting that it was skipped, with the first unaffected
- [x] 1.2 Add `scripts/mapgen/mapgen-entrypoint.sh`: arguments are passed to the pass and it exits; with `MAPGEN_CRON` unset it runs one pass; with it set it writes the crontab into the work area and runs the scheduler in the foreground (spec `mapgen-container`: one pass per start; a schedule runs the pass). Verify: `mapgen-entrypoint.sh --help` and `--check-config` behave as before, an unset `MAPGEN_CRON` reproduces the previous single-pass behaviour, and a set one starts the scheduler

## 2. The image

- [x] 2.1 Install the scheduler in `scripts/mapgen/Dockerfile`: select the binary by `TARGETARCH`, verify it against the pinned checksum for that architecture, install it, and keep the non-root user and read-only root filesystem (spec `mapgen-container`: the scheduled mode needs no privileges). Verify: the image builds for `linux/amd64`, `docker run --rm --entrypoint supercronic osmscout-mapgen:test -version` reports the pinned version, and the checksum check fails the build when the pinned value is wrong
- [x] 2.2 Ship the two scripts and make the entry point script the image's entry point, keeping `MAPGEN_TYPEFILE` and the `mapgen` user (spec `mapgen-container`: entry point semantics). Verify: `docker run --rm --read-only ... osmscout-mapgen:test --check-config` still answers `config OK`, and with `MAPGEN_CRON` set the container's process is the scheduler
- [x] 2.3 Add `util-linux` explicitly to the runtime packages so `flock` is present (spec `mapgen-container`: overlapping passes are skipped). Verify: `docker run --rm --entrypoint sh osmscout-mapgen:test -c 'command -v flock'` prints a path

## 3. Compose and documentation

- [x] 3.1 Take the `mapgen` service's restart policy from the environment (`MAPGEN_RESTART`, default `no`) and document the scheduled mode next to the service (spec `mapgen-container`: a schedule runs the pass). Verify: `docker compose config` renders `restart: "no"` with nothing set and the configured value with `MAPGEN_RESTART=unless-stopped`
- [x] 3.2 Document the scheduled mode in section 5 of `Documentation/MapRepository.md`: the variable, an example, that a scheduled container stays up, the time zone it uses, how a frequent schedule interacts with the refresh gates, and that overlapping passes are skipped (spec `mapgen-container`: scheduled operation). Verify: the example, applied to the compose file, produces a container that runs passes on the given schedule, and each command in the section runs as written
- [x] 3.3 Record in `TODO.md` what this leaves open: the checksum pins having to be updated with the scheduler, and the scheduler binary being fetched from GitHub releases at build time (spec `mapgen-container`: scheduled operation). Verify: both entries name the condition and what would close it

## 4. Smoke checks on the runner

- [x] 4.1 Add a scheduled-mode smoke check with an expression that fires within seconds, asserting that a pass ran, that its output reached the container log, and that the container is still running afterwards (spec `mapgen-container`: the schedule runs the pass). Verify: on the runner the check passes, and it fails if the entry point ignores the variable
- [x] 4.2 Add a smoke check for an unusable expression, asserting a non-zero exit and no pass (spec `mapgen-container`: an unusable expression stops the start). Verify: on the runner the container stops with a failing status within a few seconds
- [x] 4.3 Keep the existing single-pass and config-check smoke checks unchanged and green, so the opt-in property is demonstrated rather than asserted (spec `mapgen-container`: one pass per start). Verify: on the runner those checks still pass in the same run as the scheduled-mode ones
- [x] 4.4 Fix the scheduled-mode check's expression after the first runner run: a six-field expression keeps the minute-first order and adds a year instead of a seconds field, so `*/5 * * * * *` meant "every five minutes" and the check waited in vain (run `35457238892`, step 12). The check now uses the seven-field form `*/5 * * * * * *`, and the documentation states the three field counts (`min hour dom month dow`; `min hour dom month dow year`; `sec min hour dom month dow year`) instead of implying that a leading seconds field works in a five-field-compatible way. Verify: the scheduled-mode check passes on a runner, and the documentation's field table matches what the check uses

## 5. Verification

- [x] 5.1 Validate the change artifacts (spec `mapgen-container`). Verify: `openspec validate --change mapgen-cron-schedule --strict` passes
- [x] 5.2 Verify the whole path on a runner: the image builds with the scheduler, the scheduled check runs passes and stays alive, the unusable expression fails the start, the lock skips an overlapping pass, and the single-pass mode is unchanged (spec `mapgen-container`: all scenarios of both requirements). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified locally without a Docker daemon: both new scripts were exercised with stubs - arguments win over a
configured schedule, an unset `MAPGEN_CRON` runs one pass, a set one generates the crontab (expression plus
`mapgen-pass.sh`) and hands it to the scheduler, a pass whose lock is held is skipped with a message and exit
0 while the holder is unaffected, and a failing pass keeps its exit status (tasks 1.1, 1.2). The compose
restart policy renders `no` by default and the configured value with `MAPGEN_RESTART=unless-stopped` (task
3.1), `openspec validate --strict` passes (task 5.1), and the workflow parses with every step script passing
`bash -n`.

Merged as PR #1809 (merge `5e7ec3620`). Its first runner run (`35457238892`) built the image with the scheduler and verified the checksum, passed the identity, config-check, single-pass and lock checks, and then **failed** the scheduled-mode check: supercronic read the crontab and no pass ran within the 60 second wait. The cause was the expression, not the scheduler: a six-field expression is read as `minute hour day-of-month month day-of-week year` (seconds and year are the optional fields in the cronexpr parser supercronic uses), so `*/5 * * * * *` means every five minutes, and the check would have had to wait for a minute boundary. Because the verify job failed, that run published nothing (the publish job was skipped), so `latest` still does not carry the scheduler. Task 4.4 changes the check to the seven-field form and corrects the documentation.

Verified on that run: the image builds with the scheduler and its pinned checksum (2.1), the entry point starts the scheduler as the container's process (2.2), `flock` is present and a pass whose lock is held is skipped while the holder is unaffected (1.1, 2.3), and the config-check and single-pass checks still pass alongside (4.3).

Still open: the scheduled-mode and unusable-expression checks on a runner after the field fix (4.1, 4.2), the documentation commands (3.2) and the end-to-end check (5.2).

Those followed from the fix. Run `35461570774` (the pull request of the fix) shows the scheduled mode working -
the scheduler reads the crontab, fires the job four seconds later and the pass runs (`[mapgen] berlin: not due`),
with the container still up afterwards - and the unusable expression stopping the container with a non-zero
status (tasks 4.1, 4.2). The merge of the fix (`b924e0ece`, run `35462473299`) then passed the whole path:
verify with all six smoke checks, the compose job, and a publish job that pushed `:latest` and
`:20260919T185457Z` for both images and pruned both packages, so the documentation's promise that the
scheduled mode logs its passes and stays up matches what the runner showed (tasks 3.2, 5.2). The registry
answered an anonymous pull token and the tag list with five stamps plus `latest` for both images afterwards.
Everything in this change is verified.
