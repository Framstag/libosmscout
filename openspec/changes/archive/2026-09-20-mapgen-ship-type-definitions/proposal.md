# Proposal

## Why

The image ships the regeneration script and the import tool, and it bundles the type configuration the import
tool needs - but only the first file of it. `stylesheets/map.ost` pulls in three modules

```
MODULE "max_speeds"       (map.ost:76)
MODULE "contour_lines"    (map.ost:173)
MODULE "motorways"        (map.ost:174)
```

and the image contains none of them, so every import fails before it reads a single object:

```
File '/usr/local/share/osmscout/max_speeds.ost' - Cannot read size of file: No such file or directory
Error: Cannot load module '/usr/local/share/osmscout/max_speeds.ost'
   !! Cannot load type configuration!
```

The container reports this as "import failed", which points at the import rather than at the image contents,
and the first run of a new deployment is where it appears.

## What Changes

- The image SHALL contain the complete set of type definitions that the type file it bundles includes, so
  that the bundled type file loads without errors.
- A smoke check SHALL load the bundled type file inside the image, so that a missing or renamed module is
  caught by the image build's verification rather than by the first import.
- The documentation SHALL name what the image bundles (the type file and the modules it includes) and SHALL
  state that `MAPGEN_TYPEFILE` can point at an operator's own set, which has to contain those modules too.
- No change to the regeneration process, the mounts, the identity, the schedule, or the database format.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `mapgen-container`: adds the requirement that the type configuration the image bundles is complete and
  loads, next to the existing requirement about the image contents.

## Impact

Affected files and modules:

- `scripts/mapgen/Dockerfile` - installs the whole type definition set instead of one file.
- `.github/workflows/mapgen_image.yml` - a smoke check that runs the import tool against the bundled type
  file and asserts that it loads, next to the existing checks.
- `Documentation/MapRepository.md` (section 5) - names the bundled type definitions and the requirement on a
  set supplied through `MAPGEN_TYPEFILE`.
- openspec change `mapgen-ship-type-definitions` records the requirement and the verification.
- No database format, style sheet, import, build system or packaging change, and no new dependency.
