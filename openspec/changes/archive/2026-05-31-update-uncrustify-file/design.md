## Context

Current `.uncrustify` config targets Uncrustify-0.69.0_f. Installed version is 0.83.0_f with 901 options. The gap spans ~5 major versions — many options changed semantics, new options appeared, some old options deprecated. The codebase uses 2-space indent, no tabs, K&R-style braces for control flow, Allman-style braces for class/func definitions, specific pointer/reference placement (`type* name`, `type& name`).

No existing CI enforcement or developer script for formatting. CODING_STYLE.md documents conventions but no automated checks exist.

## Goals / Non-Goals

**Goals:**
- `.uncrustify` config targeting 0.83.0 that produces minimal diff on existing code
- Iterative tuning process with measurable diff-size feedback
- Developer script `scripts/format-check.sh` to apply/verify formatting
- README.md section documenting formatting workflow

**Non-Goals:**
- No source code changes — `.uncrustify` and scripts only
- No CI job implementation (future work)
- No reformatting of entire codebase in this change
- No migration of config to other formatters (clang-format, etc.)

## Decisions

### Decision 1: Start from current config, upgrade incrementally

**Rationale**: The existing config was hand-tuned for 0.69 to match project style. Starting fresh from an auto-generated 0.83 config would require re-discovering every convention. Instead, run the current config through `uncrustify --update-config` to migrate all options to 0.83 names/values, then review remaining 0.83-only options.

**Approach**:
1. `uncrustify --update-config -c .uncrustify -o .uncrustify-083` → migrate existing options
2. Diff the migrated config against original to verify no semantic shifts
3. Then add new 0.83 options (those missing from migrated config) set to match conventions

**Alternatives considered**:
- **Fresh 0.83 default + retune**: Too much work rediscovering all conventions from zero
- **Use `--update-config-with-doc`**: Not needed — `--update-config` preserves values

### Decision 2: Centroid sample approach for diff measurement

**Rationale**: Testing against the full codebase is slow. Need a representative sample that exercises all code patterns found in the project.

**Approach**:
1. Select ~20-30 files covering: headers, implementations, templates, macros, nested templates, complex expressions, enums, structs, lambdas, ranges, constructors with init lists
2. Draw from each subdirectory: `libosmscout/include/osmscout/`, `libosmscout/src/osmscout/`, `libosmscout-import/`, `Demos/`, `Tests/`
3. Run uncrustify on samples, collect unified diff stats (lines added/removed/changed)
4. Use `git diff --stat -- .uncrustify` for config change tracking
5. Final validation: run on full codebase once config stabilizes

### Decision 3: Three-pass option tuning

**Rationale**: 901 options is too many to review exhaustively. Use stratified approach:

**Pass 1 — Essential (spacing/indent/align)**: Focus on options that affect formatting of common constructs — spacing around operators, parens, braces, brackets, indentation, pointer/reference placement, line breaks. These cause the most visible diffs.

**Pass 2 — Niche (specific constructs)**: Lambda formatting, template angle brackets, C++11/17/20 constructs (structured bindings, `if constexpr`, concepts), trailing return types, `noexcept`/`override` placement.

**Pass 3 — Round-trip sanity**: Run `uncrustify -c .uncrustify` on already-formatted output. If running twice produces changes, there's a config instability. Fix until idempotent for the sample set.

### Decision 4: Script design — `scripts/format-check.sh`

**Approach**: Simple bash script wrapping uncrustify:

```bash
#!/bin/bash
# format-check.sh — apply or verify formatting with uncrustify
# Usage:
#   ./scripts/format-check.sh apply   # Format files in place
#   ./scripts/format-check.sh check   # Check formatting (exit 1 if unformatted)
#   ./scripts/format-check.sh diff    # Show diff without modifying
```

- Target file selection: all `.h`, `.cpp` under tracked dirs (exclude `build/`, `.git/`, `Android/`, `Apple/`, `subprojects/`)
- Use `git ls-files` to respect existing tracking
- `check` mode: run uncrustify, compare against original, exit 1 if any diff
- `diff` mode: show color unified diff

### Decision 5: README section placement and content

**Approach**: Add "### Code Formatting" subsection under "Documentation" in README.md:

```markdown
### Code Formatting

This project uses [Uncrustify](https://github.com/uncrustify/uncrustify) for
automatic code formatting. The configuration is in `.uncrustify` at the
project root.

To apply formatting:
```
./scripts/format-check.sh apply
```

To check if files are correctly formatted (e.g., in CI):
```
./scripts/format-check.sh check
```

Requires Uncrustify 0.83.0.
```

## Risks / Trade-offs

- **[Risk] Pass 1 may not converge if spacing conventions conflict**: e.g., `sp_arith=remove` vs explicit override need. → Mitigation: use individual overrides where general rules conflict with project convention.
- **[Risk] `--update-config` may misinterpret some 0.69 options**: Uncrustify migration isn't perfect for all edge cases. → Mitigation: diff the before/after config and spot-check known semantic changes.
- **[Risk] Sample files may not cover all edge cases**: New code patterns could get misformatted. → Mitigation: document limitation in README, encourage contributors to report formatting issues.
- **[Trade-off] No CI enforcement in scope**: Formatting can drift until CI job added. Script gives contributors local validation path.

## Resolved Questions

- **Exclude**: `Android/`, `Apple/`, `subprojects/` — these have own styling or are third-party
- **Version**: Only support uncrustify 0.83.0 (current installed)
- **Missing uncrustify**: Script should print error and exit with code 1
