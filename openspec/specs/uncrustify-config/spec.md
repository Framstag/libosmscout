## Purpose

Uncrustify configuration that enforces consistent C++20 code formatting across the libosmscout codebase, tuned for Uncrustify 0.83.0 with minimal diffs on existing code.

## Requirements

### Requirement: Config targets uncrustify 0.83.0
The `.uncrustify` file SHALL be updated to target Uncrustify 0.83.0. The version comment at the top of the file MUST reflect "Uncrustify-0.83.0_f". All options MUST use names and value ranges valid for 0.83.0.

#### Scenario: Version comment updated
- **WHEN** reading the first line of `.uncrustify`
- **THEN** it SHALL start with `# Uncrustify-0.83.0_f`

#### Scenario: All option names parse cleanly
- **WHEN** running `uncrustify -c .uncrustify --check`
- **THEN** uncrustify SHALL report no unknown or deprecated options

### Requirement: Config minimizes diff on existing code
The config MUST be tuned so formatting existing source files produces minimal changes. "Minimal" means the config SHALL NOT introduce whitespace changes in consistent, well-formatted areas of the codebase.

#### Scenario: Representative files produce acceptable diff
- **WHEN** running uncrustify with the config on a representative sample of 20-30 source files (headers + implementations across core, import, demos, tests)
- **THEN** the unified diff SHALL be reviewed and only cover areas where formatting was genuinely inconsistent or where the old config was ambiguous

#### Scenario: Round-trip stability
- **WHEN** running uncrustify twice on the same source file with the same config
- **THEN** the second pass SHALL produce identical output to the first pass (config is idempotent)

### Requirement: Config migrates from 0.69 base
The config SHALL start from the existing 0.69 file, migrated via `uncrustify --update-config` to 0.83.0 format, then augmented with new options. This preserves existing formatting conventions.

#### Scenario: Migration preserves existing intent
- **WHEN** diffing the migrated config against the original
- **THEN** all previously explicit option values SHALL carry over correctly (no semantic drift introduced by the migration tool)

### Requirement: Exclude non-owned code paths
The config SHALL NOT apply to directories containing third-party or separately-styled code: `Android/`, `Apple/`, `subprojects/`, `build/`, `.git/`.

#### Scenario: Exclusions verified
- **WHEN** running uncrustify recursively with the config
- **THEN** files under excluded directories SHALL NOT be processed or SHALL be excluded by the script

### Requirement: New 0.83-specific options match project conventions
All uncrustify options introduced between 0.69 and 0.83 that are relevant to the project's C++20 codebase SHALL be set to values consistent with CODING_STYLE.md conventions (2-space indent, no tabs, K&R braces for control flow, Allman braces for class/func, pointer/reference on left, no spaces inside parens, spaces around binary operators).

#### Scenario: Spacing options reviewed
- **WHEN** listing all spacing options in 0.83.0 that did not exist in 0.69.0
- **THEN** each option SHALL be set to `force`, `remove`, `add`, or `ignore` as appropriate to match project coding style

#### Scenario: Indentation options reviewed
- **WHEN** listing all indentation options in 0.83.0
- **THEN** each option SHALL be set to match the 2-space, no-tabs convention documented in CODING_STYLE.md