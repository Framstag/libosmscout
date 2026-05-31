## Purpose

Developer tooling (`scripts/format-check.sh`) for applying and verifying uncrustify formatting on the libosmscout codebase.

## Requirements

### Requirement: Script applies formatting
The `scripts/format-check.sh` script SHALL support an `apply` command that runs uncrustify on all tracked source files in-place, overwriting files with formatted output.

#### Scenario: Apply formats source files
- **WHEN** running `./scripts/format-check.sh apply`
- **THEN** all tracked `.h` and `.cpp` files (excluding `Android/`, `Apple/`, `subprojects/`, `build/`, `.git/`) SHALL be reformatted by uncrustify with the project's `.uncrustify` config
- **AND** the script SHALL exit with code 0 on success

### Requirement: Script checks formatting without modifying
The script SHALL support a `check` command that verifies all source files are correctly formatted without modifying them.

#### Scenario: Check passes when files are formatted
- **WHEN** running `./scripts/format-check.sh check` after `apply`
- **THEN** the script SHALL exit with code 0 and print nothing or a success message

#### Scenario: Check fails when files are unformatted
- **WHEN** running `./scripts/format-check.sh check` on files that differ from uncrustify output
- **THEN** the script SHALL exit with code 1 and print which files are unformatted

### Requirement: Script shows diff
The script SHALL support a `diff` command that shows the changes uncrustify would make, without modifying files.

#### Scenario: Diff shows pending changes
- **WHEN** running `./scripts/format-check.sh diff` on files that differ from uncrustify output
- **THEN** the script SHALL print a unified diff of each file's current content vs formatted content

### Requirement: Script fails gracefully without uncrustify
If uncrustify is not installed or not in PATH, the script SHALL print an error message and exit with code 1.

#### Scenario: Missing uncrustify detected
- **WHEN** running any script command without uncrustify installed
- **THEN** the script SHALL print `Error: uncrustify not found. Install Uncrustify 0.83.0.` to stderr
- **AND** exit with code 1

### Requirement: Script respects excluded directories
The script SHALL exclude all files under `Android/`, `Apple/`, `subprojects/`, `build/`, and `.git/` from formatting operations.

#### Scenario: Excluded files not modified
- **WHEN** running `./scripts/format-check.sh apply`
- **THEN** no files under excluded directories SHALL be modified