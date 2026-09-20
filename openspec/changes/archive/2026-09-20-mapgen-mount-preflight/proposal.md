# Proposal

## Why

A first run against freshly created named volumes fails immediately: Docker creates them owned by root, the
image runs as a non-root identity, and the entry point cannot write the crontab it generates into the work
area. The container exits, and with the restart policy a scheduled deployment needs it is restarted over and
over, printing nothing but

```
mapgen-entrypoint.sh: 39: cannot create /work/mapgen.crontab: Permission denied
```

which names neither the cause nor the fix. The same unseeded mount also breaks the single-pass mode, just
later and less legibly, when the pass tries to download into the work area or write into the repository.

## What Changes

- Before running, the image SHALL verify that the areas it has to write to are writable by the identity it
  runs as, and SHALL refuse to start with a message naming the identity, the areas and the command that
  fixes the ownership, when one of them is not.
- The check SHALL apply to both run modes, the single pass and the scheduled mode, and SHALL NOT apply to
  argument runs such as a configuration check, which only read.
- The message SHALL be actionable: it SHALL name the uid and gid in use and show how to give the mounted
  areas that ownership, including the case of a volume Docker created.
- The documentation SHALL state that the ownership of the work area and the repository is a prerequisite of
  both modes, not only of the first import.
- No change to what a pass does, to the mounts, to the identity the pass runs as, or to the schedule.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `mapgen-container`: adds the requirement that an area the image cannot write to is reported as an
  unusable mount instead of failing at the first write, and that the report is actionable.

## Impact

Affected files and modules:

- `scripts/mapgen/mapgen-entrypoint.sh` - the preflight check, before either run mode starts.
- `.github/workflows/mapgen_image.yml` - a smoke check that runs the image against a work area owned by root
  and asserts the actionable message, next to the existing single-pass and scheduled-mode checks.
- `Documentation/MapRepository.md` (section 5) - the mount table and the "Running it on a schedule" part
  state that the ownership is required by both modes, with the one-time command.
- No database format, style sheet, import, build system or packaging change, and no new dependency.
