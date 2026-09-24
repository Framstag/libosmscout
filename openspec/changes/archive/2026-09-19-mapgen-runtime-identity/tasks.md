# Tasks

## 1. The image's documented identity

- [x] 1.1 Create the image's group and user with explicit numeric ids (gid 1000 and uid 1000), so the identity the documentation states is what the image guarantees (spec `mapgen-container`: the default identity is the documented one). Verify: `docker build -f scripts/mapgen/Dockerfile` succeeds and `docker run --rm --entrypoint id osmscout-mapgen:test -u` and `-g` both print `1000`
- [x] 1.2 Assert both ids in the image smoke checks, next to the existing non-root check (spec `mapgen-container`: the default identity is the documented one). Verify: the check fails if either id differs, and passes on the built image

## 2. The orchestration's identity

- [x] 2.1 State the runtime identity of the `mapgen` service as `user: "${PUID:-1000}:${PGID:-1000}"`, leaving the serving service untouched (spec `mapgen-container`: the default identity is the documented one; the serving container is unaffected). Verify: `docker compose -f scripts/mapgen/docker-compose.yml config` renders `user: 1000:1000` with nothing set and `user: 1234:1234` with `PUID=1234 PGID=1234`, and the serving service renders no `user`
- [x] 2.2 Run one gated pass as an overridden identity in the CI smoke checks and assert that a file it created in the mounted area carries that identity (spec `mapgen-container`: the operator chooses the identity of the mounted areas; a configured identity still requires write access). Verify: on the runner, the pass as uid/gid 1234 succeeds against areas chowned to 1234, and `stat -c %u:%g` on a directory it created reports `1234:1234`; a run against an area it may not write fails
- [x] 2.3 Assert in the compose smoke job that the orchestration renders the configured identity (spec `mapgen-container`: the operator chooses the identity of the mounted areas). Verify: `PUID=1234 PGID=1234 docker compose ... config` reports `user: 1234:1234` for the mapgen service, while the job's own runs keep the default and still pass

## 3. Documentation and findings

- [x] 3.1 Document `PUID` and `PGID` in section 5 of `Documentation/MapRepository.md`: their default 1000, what they imply for a bind mount (owned by the operator, no ownership change needed) and for a named volume (seed it with the same ids), the `--user` equivalent for `docker run`, and why the serving container is not affected (spec `mapgen-container`: configurable runtime identity). Verify: every command in the section runs as written, and the ownership advice for a named volume matches the task 2.3 behaviour
- [x] 3.2 Record in `TODO.md` whatever this change leaves open - at least the possibility that an identity without a matching entry in `/etc/passwd` breaks a tool (the task 2.2 run is what answers it) (spec `mapgen-container`: a configured identity still requires write access). Verify: the entry names the condition and what would close it

## 4. Verification

- [x] 4.1 Validate the change artifacts (spec `mapgen-container`). Verify: `openspec validate --change mapgen-runtime-identity --strict` passes
- [x] 4.2 Verify the whole path on a runner: the image asserts uid and gid 1000, the overridden pass succeeds and writes with the overridden identity, and the compose rendering reports both the default and a configured identity (spec `mapgen-container`: all four scenarios). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified locally, because it needs no Docker daemon: the compose rendering of the runtime identity - the
same file renders `user: 1000:1000` with nothing set, `1234:1234` with `PUID=1234 PGID=1234` (and
`4321:4321` from an environment file), and no `user` for the serving service (task 2.1); the workflow
parses and every step script passes `bash -n`; `openspec validate --strict` passes (task 4.1).

All of them were then observed on a runner. The merge of PR #1808 (`e97f87b9c`, run `35456696828`) passed the
verify job - `Verify the runtime identity` asserts uid 1000 and gid 1000, and `Smoke test (single pass as an
overridden uid/gid)` runs a pass as `1234:1234` against areas chowned to 1234 and asserts that
`private/staging`, which the pass itself creates, is owned by `1234:1234` - the compose job, whose
`Verify the configured runtime identity is rendered` step asserts `1000:1000` by default, `1234:1234` with
`PUID`/`PGID` set and no `user` for the serving service, and the publish job, which pushed `:latest` and
`:20260919T173032Z` for both images (tasks 1.1, 1.2, 2.2, 2.3, 4.2). The `--user` form the documentation
shows for `docker run` is the same invocation the overridden pass uses, and the compose commands in the
section are the ones the compose job runs (task 3.1). Everything in this change is verified.
