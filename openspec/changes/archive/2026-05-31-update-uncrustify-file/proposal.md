## Why

The project has an `.uncrustify` config targeting Uncrustify-0.69.0, but the installed version is 0.83.0 (901 options vs fewer in 0.69). Many options that could fine-tune formatting are missing or stale. Parts of the codebase diverge in formatting style, and contributors lack automated tooling to align with the project's conventions. Updating the config to a modern uncrustify version and tuning it to produce minimal diffs on existing code makes formatting enforcement practical in CI and for contributors.

## What Changes

- Update `.uncrustify` from Uncrustify-0.69.0 target to 0.83.0
- Review all new/modified options added since 0.69, set them to match project style (documented in CODING_STYLE.md)
- Run uncrustify across representative sample of source files to measure diff size
- Iterate options to minimize deviations from existing code style
- Add a script for developers/CI to check formatting compliance
- No source code modifications — only the `.uncrustify` config, scripts, and README.md

## Capabilities

### New Capabilities
- `uncrustify-config`: Updated `.uncrustify` file tuned for uncrustify 0.83.0 that produces minimal diffs on existing code
- `formatting-check-script`: Developer tooling to apply/verify formatting (e.g., `scripts/format-check.sh`)

### Modified Capabilities
<!-- No existing specs change -- this is a new tooling infrastructure effort -->

## Impact

- `.uncrustify` file - complete rewrite targeting 0.83.0
- `README.md` — new section documenting uncrustify usage
- `scripts/` — new shell script for running uncrustify on the codebase
- Optional future CI job (not in scope for this change)
- No changes to any `.h`, `.cpp`, `CMakeLists.txt`, or Meson files