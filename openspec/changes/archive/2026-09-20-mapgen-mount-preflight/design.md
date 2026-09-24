# Design

## Context

See `proposal.md` - Why. The relevant facts:

- The entry point writes the crontab for the scheduled mode into the work area, and the pass writes downloads
  and intermediates there and databases into the repository. Both areas are mounts, so their ownership comes
  from the host or from Docker, not from the image.
- Docker creates a named volume owned by root; the image's user is uid 1000 (or whatever `PUID`/`PGID` say),
  so the first run against a fresh volume cannot write. The CI smoke checks seed the ownership first, which is
  why the unseeded case was never exercised.
- The root filesystem is read-only, so nothing outside the mounts can be used as a scratch location: the
  crontab has to live in the work area. supercronic takes exactly one argument, a crontab file name, and has
  no stdin form (`main.go`, `flag.NArg() != 1`), so this cannot be avoided by piping the schedule.

## Decisions

### 1. Check both areas up front, in the entry point, for the run modes only

`mapgen-entrypoint.sh` tests that it can create and remove a file in the work area and in the repository
before it starts either mode, and stops with an actionable message when it cannot: the area, the uid and gid
in use, and a `chown` command for it. A run with arguments (`--check-config`, `--help`, an explicit run) keeps
its current behaviour and is not blocked, because those arguments ask for a read-only operation.

- Alternative - let the failure happen where it happens (today's behaviour): one line of shell error, in a
  restart loop, naming nothing that the operator can act on.
- Alternative - create the areas in the image and rely on them: a mount hides whatever the image contains, so
  this cannot work for the failure that matters.
- Alternative - check only the work area: the repository is the other half of the same problem and produces
  the same kind of confusion, only later.
- Rationale: the check is three lines per area, it turns an unbounded restart loop into a single intelligible
  refusal, and it cannot hide a problem that would otherwise not exist - the areas have to be writable for a
  pass to work at all.

### 2. The report has to be actionable, including the fix for a Docker-created volume

The message names the identity (`id -u`, `id -g`) and shows the one-time command that gives the areas that
ownership, both in the compose form and with a plain `docker run`. A message that only says "permission
denied" would leave the operator exactly where the original error did.

## Risks / Trade-offs

- A container that is deliberately started to *inspect* something with an unwritable mount now stops instead
  of starting: acceptable, since the pass could not have done useful work either, and argument runs are
  exempt → the message says why it stopped.
- The check writes and removes a file in each area, so a read-only filesystem with no mounts fails the same
  way an unseeded volume does, with the same message → that is the intended diagnosis in both cases.
- With `restart: unless-stopped` the refusal repeats on every restart, as the cryptic error did; the
  difference is that each repetition is now readable → the documentation tells the operator what to do, and
  the message repeats the command.

## Migration Plan

Nothing to migrate: writable areas behave exactly as before. Existing deployments that already seeded their
volumes see no change, and new ones get a message instead of a loop.
