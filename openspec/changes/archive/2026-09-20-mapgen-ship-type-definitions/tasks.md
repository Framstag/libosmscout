# Tasks

## 1. The image contents

- [x] 1.1 Install the whole type definition set in `scripts/mapgen/Dockerfile` (`stylesheets/*.ost` into the directory the type file is read from), replacing the single file copy (spec `mapgen-container`: the bundled type configuration is complete and loads). Verify: the built image contains `map.ost` and every module it references, and the type file is found by the tool at the path the image's `MAPGEN_TYPEFILE` points to
- [x] 1.2 Add a smoke check that loads the bundled type file inside the image and asserts that no module fails to load (spec `mapgen-container`: the bundled type file loads). Verify: on the runner the check passes, and it fails if a module is removed from the image or renamed in the repository

## 2. Documentation

- [x] 2.1 Name what the image bundles, and that a set supplied through `MAPGEN_TYPEFILE` has to contain the modules as well as the type file, in section 5 of `Documentation/MapRepository.md` (spec `mapgen-container`: every module is present). Verify: the section names the bundled type definitions and the module requirement, and the module names in the text match the ones the type file includes
- [x] 2.2 Record what this leaves open in `TODO.md` if anything does (spec `mapgen-container`: a missing module is a configuration failure). Verify: the entry, if any, names the condition and what would close it. Nothing was left open: a missing module now fails the smoke check on the runner, and the message the tool prints already names the module

## 3. Verification

- [x] 3.1 Validate the change artifacts (spec `mapgen-container`). Verify: `openspec validate --change mapgen-ship-type-definitions --strict` passes
- [x] 3.2 Verify the whole path on a runner: the image builds, the smoke check loads the bundled type file, and the existing checks stay green (spec `mapgen-container`: all three scenarios). Verify: a pull request run whose steps all conclude `success`

## Verification status

Verified from the repository, since no Docker daemon is reachable here: the type file the image bundles names
exactly three modules (`grep -oE 'MODULE "[^"]+"' stylesheets/map.ost` produces `max_speeds`,
`contour_lines`, `motorways`), all three exist in `stylesheets/`, and the glob the Dockerfile now copies takes
all five type definition files in that directory, so the modules end up beside the type file. The workflow
parses with all 18 step scripts passing `bash -n`, and `openspec validate --strict` passes (tasks 1.1, 2.1,
3.1). The failure itself is from a deployment: `Error: Cannot load module
'/usr/local/share/osmscout/max_speeds.ost'` followed by `!! Cannot load type configuration!`.

Open, and to be observed on a runner: the image builds with the whole set, the smoke check loads the bundled
type file and finds every module it names, and the existing checks stay green (tasks 1.2, 3.2).

Observed on the merge's runner run, `35467420850` (merge `0b89e2220`): the `verify` job passed every step,
including `Smoke test (the bundled type configuration loads)`, the compose job passed, and the publish job pushed
the release tags. The check loads the type file with the import tool and then derives the module names from that
file and requires each of them to be present, so both a missing module and a renamed one fail the run. All six
tasks are verified.
