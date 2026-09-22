# Proposal

## Why

The regeneration container writes into two mounted areas, the repository and its work area, and it runs as
the non-root user `mapgen` with uid 1000. An operator whose repository lives on a bind mount owned by their
own account, or who uses a volume created by another tool, therefore has to change ownership to
`1000:1000` (the documented procedure does exactly that) even though the runtime identity is the only
reason. Letting the operator choose the uid and gid the pass runs as removes that step and makes the
ownership of the mounted areas and the process match by construction.

## What Changes

- The orchestration SHALL let the operator set the uid and gid the regeneration pass runs as, through the
  conventional `PUID` and `PGID` environment variables, defaulting to `1000` each so that an operator who
  sets nothing keeps today's behaviour.
- The image SHALL create its non-root user with that same documented identity (uid 1000 and gid 1000), so
  the default and the documented identity are the same fact rather than a coincidence.
- The mounted areas SHALL be written with the configured identity, so a repository mounted from a directory
  owned by the operator needs no ownership change.
- The serving container SHALL keep the identity of its base image: it only reads the repository, and its own
  runtime directories belong to that image's user.
- Documentation SHALL state the variables, their default, the ownership they imply for a bind mount and for
  a named volume, and the equivalent for a plain `docker run` (`--user`).
- No change to what the images contain, what the pass does, the mount points, or the reading side.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `mapgen-container`: adds the requirement that the runtime identity of the regeneration pass is
  configurable, and that the image's own user has the same documented identity as the default.

## Impact

Affected files and modules:

- `scripts/mapgen/docker-compose.yml` - the `mapgen` service states its runtime identity from `PUID`/`PGID`
  with the current identity as the default.
- `scripts/mapgen/Dockerfile` - the non-root user and its group are created with explicit numeric ids, so
  "uid 1000, gid 1000" is what the image guarantees rather than what `useradd` happened to pick.
- `.github/workflows/mapgen_image.yml` - the smoke checks assert both ids of the image's user, run one
  gated pass as an overridden identity and assert that the files it created carry that identity, and the
  compose job asserts that the orchestration renders the configured identity.
- `Documentation/MapRepository.md` (section 5) - the variables, the default, the ownership they imply, and
  the `--user` equivalent.
- No database format, style sheet, import, build system or packaging change, and no new dependency.
