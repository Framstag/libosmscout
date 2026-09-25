# Design

## Context

See `proposal.md` - Why. Facts that shape the approach:

- The image runs the pass as a non-root user (uid 1000 by default, configurable since `mapgen-runtime-identity`)
  with `read_only: true` for the root filesystem, and the only writable places are `/work` and `/repository`.
- A distribution `cron` daemon needs root and writes `/var/spool/cron` and `/run/crond.pid`, so it cannot run
  here without giving up both properties.
- `supercronic` (v0.2.49) is a static binary that reads a crontab, needs no privileges, writes no spool files,
  logs job output to stdout/stderr, supports second-resolution schedules and time zones, and does not start a
  job again while its previous run is still going.
- The pass already has a refresh gate per import, so a frequent schedule is cheap: occurrences that arrive
  before an import is due produce no download and no import.

## Decisions

### 1. supercronic, opt-in through `MAPGEN_CRON`, single-pass mode kept

The entry point (`scripts/mapgen/mapgen-entrypoint.sh`) reads `MAPGEN_CRON`:

| situation | behaviour |
|---|---|
| arguments given (`--check-config`, `--help`, an explicit run) | pass them to the pass, run once, exit |
| `MAPGEN_CRON` unset | one pass, exit - today's behaviour, so an external scheduler keeps working |
| `MAPGEN_CRON` set | write the crontab into the work area and run supercronic as the foreground process |

- Alternative - the distribution `cron`: rejected, it needs root and a writable `/var/spool/cron`, which
  contradicts the non-root and read-only-rootfs requirements that the same spec asks for.
- Alternative - always scheduled: rejected, it would break the documented external-scheduler use and every
  existing compose setup, which relies on the container exiting.
- Alternative - an interval variable with a sleep loop: no cron expression, so "weekdays at 03:00" would be
  impossible to express.
- Rationale: the new mode is a mode, not a new image behaviour; nothing that works today changes.

### 2. The scheduler is installed by architecture with a pinned checksum

The Dockerfile takes `TARGETARCH`, maps it to the matching release asset (`supercronic-linux-amd64`,
`supercronic-linux-arm64`, both known: sha256 `a53ae236...430c1` and `02aa0cb2...9dd5`), downloads it and
verifies it with `sha256sum -c` before installing.

- Alternative - download without verifying: accepted for a base image, not for a binary we then execute as the
  container's main process.
- Alternative - a package from the distribution: the package is the unprivileged-unsuitable `cron`.
- Residual: the checksum is pinned in the Dockerfile, so upgrading the scheduler is a deliberate two-line
  change (version and hash per architecture), recorded in `TODO.md`.

### 3. Overlap is prevented by an exclusive lock, not by hope

`scripts/mapgen/mapgen-pass.sh` runs `mapgen.sh` under `flock -n` on a lock file in the work area and exits
successfully with a message when the lock is held. The scheduled occurrences and any external trigger both go
through it, so an externally triggered pass during a scheduled one is skipped instead of racing on the
staging directory.

- Alternative - rely on supercronic's own serialization: it covers its own schedule only, not an external
  trigger, and the external trigger is a documented way to run a pass.
- Alternative - no guard: two passes on the same repository can interleave in `private/`, where the script
  assumes it is the only writer.
- Rationale: one `flock` call, no privilege, and the failure mode becomes a log line instead of a corrupted
  staging area.

### 4. The compose file leaves the restart policy to the operator

The `mapgen` service takes its restart policy from the environment (`restart: "${MAPGEN_RESTART:-no}"`),
because the two modes need opposite values: a single-pass container must stay down when it exits, while a
scheduled one has to be restarted if it dies. Compose cannot express "if `MAPGEN_CRON` is set, then ...", so
the value is stated explicitly and documented next to the service.

## Risks / Trade-offs

- The scheduled mode changes the container from a job into a service: a compose user who sets `MAPGEN_CRON`
  but leaves `MAPGEN_RESTART` unset has a container that stays up and is never restarted after a crash →
  the documentation shows both settings together, and the compose file's comment repeats it.
- A schedule that fires more often than a pass takes leads to skipped occurrences (loudly logged) → that is
  the intended behaviour and is stated in the spec scenario; the refresh gates make a fast schedule cheap
  rather than harmful.
- The scheduler binary is fetched at build time from GitHub releases; a build without network access cannot
  run in the same way as before → the CI image build is the check, and the alternative is vendoring the
  binary, which would add 14 MB to the repository.
- `TZ` decides when an expression such as `0 3 * * *` fires, and the image does not set one by default → the
  documentation states that the variable must be set (or the expression kept in UTC), and the compose example
  sets it.
- The crontab is written into the work area, which is transient by design → nothing depends on it persisting;
  it is regenerated at every start.

## Migration Plan

Nothing to migrate: with `MAPGEN_CRON` unset the container behaves exactly as before. An operator with an
external scheduler can move one import at a time by setting `MAPGEN_CRON` and `MAPGEN_RESTART=unless-stopped`
in the compose service and removing the host-side trigger.
