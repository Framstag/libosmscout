# Tasks

## 1. The two new scripts

- [x] 1.1 Add `scripts/mapgen/mapgen-pass.sh`: one pass under an exclusive lock in the work area, reporting and exiting successfully when the lock is held by another pass (spec `mapgen-container`: overlapping passes are skipped). Verify: two concurrent invocations show one pass running and the second reporting that it was skipped, with the first unaffected
- [x] 1.2 Add `scripts/mapgen/mapgen-entrypoint.sh`: arguments are passed to the pass and it exits; with `MAPGEN_CRON` unset it runs one pass; with it set it writes the crontab into the work area and runs the scheduler in the foreground (spec `mapgen-container`: one pass per start; a schedule runs the pass). Verify: `mapgen-entrypoint.sh --help` and `--check-config` behave as before, an unset `MAPGEN_CRON` reproduces the previous single-pass behaviour, and a set one starts the scheduler

## 2. The image

- [ ] 2.1 Install the scheduler in `scripts/mapgen/Dockerfile`: select the binary by `TARGETARCH`, verify it against the pinned checksum for that architecture, install it, and keep the non-root user and read-only root filesystem (spec `mapgen-container`: the scheduled mode needs no privileges). Verify: the image builds for `linux/amd64`, `docker run --rm --entrypoint supercronic osmscout-mapgen:test -version` reports the pinned version, and the checksum check fails the build when the pinned value is wrong
- [ ] 2.2 Ship the two scripts and make the entry point script the image's entry point, keeping `MAPGEN_TYPEFILE` and the `mapgen` user (spec `mapgen-container`: entry point semantics). Verify: `docker run --rm --read-only ... osmscout-mapgen:test --check-config` still answers `config OK`, and with `MAPGEN_CRON` set the container's process is the scheduler
- [ ] 2.3 Add `util-linux` explicitly to the runtime packages so `flock` is present (spec `mapgen-container`: overlapping passes are skipped). Verify: `docker run --rm --entrypoint sh osmscout-mapgen:test -c 'command -v flock'` prints a path

## 3. Compose and documentation

- [x] 3.1 Take the `mapgen` service's restart policy from the environment (`MAPGEN_RESTART`, default `no`) and document the scheduled mode next to the service (spec `mapgen-container`: a schedule runs the pass). Verify: `docker compose config` renders `restart: "no"` with nothing set and the configured value with `MAPGEN_RESTART=unless-stopped`
- [ ] 3.2 Document the scheduled mode in section 5 of `Documentation/MapRepository.md`: the variable, an example, that a scheduled container stays up, the time zone it uses, how a frequent schedule interacts with the refresh gates, and that overlapping passes are skipped (spec `mapgen-container`: scheduled operation). Verify: the example, applied to the compose file, produces a container that runs passes on the given schedule, and each command in the section runs as written
- [x] 3.3 Record in `TODO.md` what this leaves open: the checksum pins having to be updated with the scheduler, and the scheduler binary being fetched from GitHub releases at build time (spec `mapgen-container`: scheduled operation). Verify: both entries name the condition and what would close it

## 4. Smoke checks on the runner

- [ ] 4.1 Add a scheduled-mode smoke check with an expression that fires within seconds, asserting that a pass ran, that its output reached the container log, and that the container is still running afterwards (spec `mapgen-container`: the schedule runs the pass). Verify: on the runner the check passes, and it fails if the entry point ignores the variable
- [ ] 4.2 Add a smoke check for an unusable expression, asserting a non-zero exit and no pass (spec `mapgen-container`: an unusable expression stops the start). Verify: on the runner the container stops with a failing status within a few seconds
- [ ] 4.3 Keep the existing single-pass and config-check smoke checks unchanged and green, so the opt-in property is demonstrated rather than asserted (spec `mapgen-container`: one pass per start). Verify: on the runner those checks still pass in the same run as the scheduled-mode ones

## 5. Verification

- [x] 5.1 Validate the change artifacts (spec `mapgen-container`). Verify: `openspec validate --change mapgen-cron-schedule --strict` passes
- [ ] 5.2 Verify the whole path on a runner: the image builds with the scheduler, the scheduled check runs passes and stays alive, the unusable expression fails the start, the lock skips an overlapping pass, and the single-pass mode is unchanged (spec `mapgen-container`: all scenarios of both requirements). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified locally without a Docker daemon: both new scripts were exercised with stubs - arguments win over a
configured schedule, an unset `MAPGEN_CRON` runs one pass, a set one generates the crontab (expression plus
`mapgen-pass.sh`) and hands it to the scheduler, a pass whose lock is held is skipped with a message and exit
0 while the holder is unaffected, and a failing pass keeps its exit status (tasks 1.1, 1.2). The compose
restart policy renders `no` by default and the configured value with `MAPGEN_RESTART=unless-stopped` (task
3.1), `openspec validate --strict` passes (task 5.1), and the workflow parses with every step script passing
`bash -n`.

Open, because this machine has no reachable Docker daemon (Rancher Desktop's socket disappeared earlier in
the session): the image build with the scheduler and its checksum verification, the entry point as the
container's entry point, `flock` being present, the three new smoke checks on a runner, the documentation
commands, and the end-to-end check (tasks 2.1, 2.2, 2.3, 3.2, 4.1, 4.2, 4.3, 5.2).
