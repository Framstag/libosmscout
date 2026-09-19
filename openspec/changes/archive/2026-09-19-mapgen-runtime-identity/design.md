# Design

## Context

See `proposal.md` - Why. Facts that shape it:

- The image runs the pass as `mapgen`, uid 1000, created by `useradd --system --uid 1000 -o`, so its group id
  is whatever `useradd` chose rather than a documented value; only the uid is asserted by the CI smoke check.
- The compose file mounts two writable areas (`/repository`, `/work`) and one read-only area (`/config`), and
  sets `read_only: true` for the root filesystem.
- `Documentation/MapRepository.md` currently tells the operator to `chown -R 1000:1000` the volumes, which is
  exactly the step that becomes unnecessary once the identity is configurable.
- The serving image is nginx-based and writes `/var/cache/nginx`, owned by that image's user.

## Decisions

### 1. The runtime identity is set by the container runtime, not by the entry point

`docker-compose.yml` states `user: "${PUID:-1000}:${PGID:-1000}"` for the `mapgen` service.

- Alternative - remap the user inside the entry point (the pattern used by images that accept `PUID`/`PGID`
  as container environment): rejected, it needs the process to start as root and to write `/etc/passwd`,
  which the read-only root filesystem and the "no root execution" requirement forbid. It would also mean two
  privilege levels in one container.
- Alternative - keep only `--user` for `docker run` users and document it: rejected, the compose file is the
  documented entry point, and it would still need the ownership step.
- Rationale: the runtime sets credentials without any cooperation from the image, the root filesystem stays
  read-only, and `PUID`/`PGID` are the names operators already look for; `${UID}`/`${GID}` are shell builtins
  that most shells do not export, so they silently resolve to nothing.

### 2. The image's own user gets the same documented ids

The Dockerfile creates `groupadd --gid 1000 mapgen` before the user, so the image user is uid 1000 and gid
1000 rather than uid 1000 and an implicit group. Without this, the compose default would be a subtle change
from today's plain `USER mapgen` (uid 1000 with whatever gid `useradd` picked) and the documented identity
would not be a fact about the image.

### 3. The serving container keeps its base image's identity

`nginx:1.27-alpine` runs nginx with its own uid and writes `/var/cache/nginx`, which that image owns. The
serving container only reads the repository volume, so a configurable identity would buy nothing and would
break startup. Stated in the spec so the asymmetry is deliberate rather than an oversight.

## Risks / Trade-offs

- A numeric identity without a matching entry in `/etc/passwd` can affect tools that resolve user
  information; the smoke checks therefore run a pass as an overridden identity on the CI runner, so the
  behaviour with a foreign uid is exercised rather than assumed → if that turns out to be a problem, the
  fallback is to require the operator to use an identity that exists in the image (uid 1000 or the ids of a
  named user), which the documentation would state.
- A configured identity that cannot write the mounted areas fails at the first write, with the underlying
  error → the documentation states the requirement, and the pass never falls back to another identity.
- The `PUID`/`PGID` names are a convention, not an engine feature → the documentation names them and shows
  the `--user` equivalent for `docker run`.

## Migration Plan

Nothing to migrate: an operator who sets nothing keeps uid 1000 and gid 1000, and the ownership step that
the documentation described stays valid for named volumes. Operators who want their own ids stop needing it.
