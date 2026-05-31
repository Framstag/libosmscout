## 1. Config Migration

- [x] 1.1 Migrate existing `.uncrustify` config to 0.83.0 using `uncrustify --update-config -c .uncrustify -o .uncrustify-083` and verify no semantic drift (spec: uncrustify-config)
- [x] 1.2 Replace `.uncrustify` with migrated file, update version comment to `# Uncrustify-0.83.0_f`, verify `uncrustify -c .uncrustify --check` passes (spec: uncrustify-config)

## 2. Option Tuning — Pass 1: Essential

- [x] 2.1 Compare in 0.83 all spacing options (`sp_*`) against project conventions (2-space indent, K&R control braces, Allman class/func braces, pointer/reference on left, no spaces inside parens, spaces around binary ops) and adjust to minimize diff (spec: uncrustify-config)
- [x] 2.2 Compare all indentation options (`indent_*`) against project conventions, ensuring 2-space, no-tabs is enforced and all alignment options match project patterns (spec: uncrustify-config)

## 3. Option Tuning — Pass 2: Niche

- [x] 3.1 Review options for C++11/17/20 constructs (lambdas, structured bindings, `if constexpr`, concepts, trailing return types, `noexcept`/`override`) and set to match project style (spec: uncrustify-config)
- [x] 3.2 Review options for template angle brackets, macros, preprocessor, and edge-case constructs (spec: uncrustify-config)

## 4. Diff Validation

- [x] 4.1 Select representative sample of 20-30 source files across core, import, demos, tests and run uncrustify — collect diff stats (spec: uncrustify-config)
- [x] 4.2 Iterate config adjustments until diffs are minimal and only cover genuinely inconsistent areas; verify round-trip stability (second pass = first pass) (spec: uncrustify-config)
- [x] 4.3 Run uncrustify on full codebase as final validation (spec: uncrustify-config)

## 5. Formatting Script

- [x] 5.1 Create `scripts/format-check.sh` with `apply`, `check`, and `diff` commands; use `git ls-files` for source file discovery, exclude `Android/`, `Apple/`, `subprojects/`, `build/`, `.git/`; gracefully handle missing uncrustify (spec: formatting-check-script)

## 6. Documentation

- [x] 6.1 Add "### Code Formatting" section to README.md documenting uncrustify setup and script usage (design: Decision 5)