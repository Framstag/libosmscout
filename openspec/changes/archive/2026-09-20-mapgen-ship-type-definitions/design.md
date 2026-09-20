# Design

## Context

See `proposal.md` - Why. Facts:

- `stylesheets/` holds five type definition files: `map.ost` (the complete set, 100 KB), `max_speeds.ost`,
  `contour_lines.ost`, `motorways.ost` and `basemap.ost` (a separate set for the basemap import). All are
  small text files; together they are around 110 KB.
- `map.ost` reaches the other three with `MODULE "<name>"`, and the import tool resolves a module to
  `<directory of the type file>/<name>.ost` - which is why the failure names
  `/usr/local/share/osmscout/max_speeds.ost`.
- `MAPGEN_TYPEFILE` is the environment variable the script passes to the import tool as `--typefile`, so an
  operator can point at their own set; that set has the same requirement, since the modules are resolved
  relative to it.

## Decisions

### 1. Install the whole set with one copy

The Dockerfile copies `stylesheets/*.ost` into `/usr/local/share/osmscout/` instead of the single `map.ost`,
so the directory holds the type file and its modules side by side, exactly as the repository keeps them.

- Alternative - list the three modules explicitly: it works today and breaks the day someone adds a fourth
  module, which is precisely the failure this change repairs.
- Alternative - copy the whole `stylesheets/` directory: it would also ship the `.oss` style sheets, which
  the import tool never reads and which are several megabytes.
- Rationale: one line, no list to keep in step, and the image ends up with the same type sets the repository
  has.

### 2. The smoke check loads the type file, rather than checking files exist

A check that the import tool loads the bundled type file catches both a missing module and a module that was
renamed in the repository, and it does not depend on knowing the module names. It runs the tool with only
`--typefile`, so it fails fast at configuration time and never touches source data (which the CI has none of
anyway).

- Alternative - assert that four specific files exist: cheaper, but blind to a new module, which is the
  failure that reached a deployment.
- Rationale: the check has to fail for the reason the change is about.

## Risks / Trade-offs

- The bundled set is whatever `stylesheets/` holds at build time, so an import can produce a database whose
  type configuration differs from the one an operator's client was built with → unchanged from before (the
  bundled file was already taken from the build's revision), and `db.json` records the type-config version.
- An operator overriding `MAPGEN_TYPEFILE` must copy the modules too → stated in the documentation next to the
  variable; the failure message names the missing module, so it is diagnosable.
- The image grows by about 105 KB → not worth a trade-off.

## Migration Plan

Nothing to migrate: images built from the fixed Dockerfile simply have a type configuration that loads. A
deployment that worked around the problem by mounting its own set can keep doing so, or drop the override and
use the bundled one.
