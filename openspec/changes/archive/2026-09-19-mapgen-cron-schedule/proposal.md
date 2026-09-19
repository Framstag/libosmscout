# Proposal

## Why

The container was designed for an external scheduler that starts it once per run and lets it exit: its entry
point does one pass and terminates, and the image deliberately contains no scheduler. With the compose setup
that assumption costs an external crontab or timer on the host, a restart policy that has to be right, and a
schedule that is invisible from the stack itself - the operator has to keep the host and the compose file in
step for what is really one piece of configuration. Running the pass on a schedule from inside the image
removes that coupling: the schedule becomes an environment variable of the same service that already knows
the manifest, the repository and the identity to run as.

## What Changes

- The image SHALL be able to run the pass on a schedule given as a cron expression through an environment
  variable, and SHALL contain a scheduler that works without root privileges and without a writable root
  filesystem, since the image runs as a non-root user with a read-only root filesystem.
- The scheduled mode SHALL be opt-in: with the variable unset the current behaviour stays, one pass per start
  and then exit, so an external scheduler remains supported.
- Two passes SHALL NOT overlap, whether they come from the schedule or from an external trigger.
- The scheduled mode SHALL reject an unusable expression at start instead of running nothing.
- The schedule SHALL be documented with the container, including the time zone it is evaluated in, and how
  the refresh gates of the manifest interact with a frequent schedule.
- No change to what a pass does, to the mounts, to the identity the pass runs as, or to the read-only root
  filesystem.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `mapgen-container`: the requirement that the entry point runs exactly one pass and that the image contains
  no scheduler becomes an opt-in decision between a single pass and a scheduled mode, and the scheduled mode
  brings its own requirement (non-overlapping passes, fail fast on an unusable expression).

## Impact

Affected files and modules:

- `scripts/mapgen/Dockerfile` - installs the scheduler (a static binary selected by target architecture and
  verified against a pinned checksum), ships the entry point that decides between the two modes, and keeps
  the non-root user and the read-only root filesystem.
- `scripts/mapgen/mapgen-entrypoint.sh` (new) - chooses between a single pass, a pass with arguments, and the
  scheduled mode, and generates the crontab from the expression.
- `scripts/mapgen/mapgen-pass.sh` (new) - one pass under an exclusive lock, so a scheduled pass and an
  externally triggered one cannot overlap.
- `scripts/mapgen/docker-compose.yml` - the restart policy becomes part of the configuration (the scheduled
  mode has to stay up), with the mode documented next to the service.
- `.github/workflows/mapgen_image.yml` - the smoke checks gain the scheduled mode: a schedule with second
  resolution runs a pass and keeps the container alive, and an unusable expression fails the start.
- `Documentation/MapRepository.md` (section 5) - the scheduled mode, its variable, the time zone, the
  interaction with the refresh gates, and the restart policy it needs.
- `TODO.md` - what this leaves open, for example the checksum pin having to be updated when the scheduler is
  upgraded.
- No database format, style sheet, import or build system change; no new runtime dependency beyond the
  scheduler binary.
