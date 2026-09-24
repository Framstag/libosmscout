# Tasks

## 1. The preflight check

- [x] 1.1 Add the writability check to `scripts/mapgen/mapgen-entrypoint.sh` for the work area and the repository, in both run modes and not for argument runs, with a message naming the area, the uid and gid in use and a command that fixes the ownership (spec `mapgen-container`: an unwritable area is reported, not discovered later). Verify: the scripts run with stubs against a writable area (passes, leaves no test file behind), against an area owned by another identity (stops, naming that area), and with `--check-config` against an unwritable area (not blocked)
- [x] 1.2 Extend the smoke checks with the unseeded case: a work area owned by root, started in the scheduled mode, has to stop with the actionable message (spec `mapgen-container`: a work area owned by another identity). Verify: on the runner the check passes, and it fails if the message loses the area name, the identity or the `chown` command
- [x] 1.3 Keep the existing smoke checks green: single pass, scheduled mode, unusable schedule and the argument runs (spec `mapgen-container`: writable areas start normally). Verify: on the runner those checks pass in the same run as the new one

## 2. Documentation

- [x] 2.1 State in section 5 of `Documentation/MapRepository.md` that the ownership of the work area and the repository is a prerequisite of both modes, with the one-time command, and that the entry point reports an unusable area with that same command (spec `mapgen-container`: an unwritable area is reported, not discovered later). Verify: the mount table and the scheduled-mode part both carry the prerequisite, and the command in the message matches the command in the documentation
- [x] 2.2 Record what this leaves open in `TODO.md` if anything does (spec `mapgen-container`: writable areas start normally). Verify: the entry, if any, names the condition and what would close it. Nothing was left open: the message is the whole remedy, and the restart policy repeating it is the documented behaviour of `restart: unless-stopped`

## 3. Verification

- [x] 3.1 Validate the change artifacts (spec `mapgen-container`). Verify: `openspec validate --change mapgen-mount-preflight --strict` passes
- [x] 3.2 Verify the whole path on a runner: the unseeded work area stops the container with the actionable message, the writable cases still start and pass their checks, and the argument runs stay unaffected (spec `mapgen-container`: all four scenarios). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified locally with stub binaries and directory permissions, because no Docker daemon is reachable on this
machine (Rancher Desktop's socket disappeared earlier in the session): a writable work area and repository
start normally in both modes and leave no probe file behind; a repository that the process may not write to
stops the container with `the repository (/repo) is not writable by uid 1000, gid 1000` plus the command; a
work area with the same problem stops it with the same message naming `/work`; and a run with an argument
(`--check-config`) is not blocked by an unwritable work area. The workflow parses with all 19 step scripts
passing `bash -n`, and `openspec validate --strict` passes (tasks 1.1, 3.1).

Open, and to be observed on a runner: the two new smoke checks - an unseeded work area in the scheduled mode
has to stop with the actionable message, and a read-only run against it has to succeed - alongside the
existing checks (tasks 1.2, 1.3, 3.2).

Observed on the merge's runner run, `35466932646` (merge `afdebdf4e`): the `verify` job passed every step,
including `Smoke test (an unseeded work area is reported)` and `Smoke test (a read-only run is not blocked by
an unseeded area)`, the compose job passed, and the publish job pushed the release tags - so the unseeded case
stops the container with the message and the read-only case is not blocked, exactly as the spec requires. All
seven tasks are verified.
