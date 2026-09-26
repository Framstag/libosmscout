# symbol-scan-tool Specification (delta)

## ADDED Requirements

### Requirement: Pattern enumeration API

The library SHALL expose the names of all pattern fills used by a loaded stylesheet, so a client can iterate over the full set of patterns without knowing them in advance.

#### Scenario: Enumerate patterns of a loaded stylesheet
- **WHEN** a stylesheet defining area fills with patterns "landuse_apiary" and "landuse_forest" has been loaded
- **THEN** the client can retrieve the complete list of pattern names, containing exactly "landuse_apiary" and "landuse_forest"

#### Scenario: Stylesheet without patterns
- **WHEN** a stylesheet defining no pattern fills has been loaded
- **THEN** the enumeration returns an empty list

### Requirement: Render pattern fills via Cairo backend

The tool SHALL render every pattern fill of the loaded stylesheet standalone onto a plain background using the Cairo backend, and SHALL write one PNG file per pattern named `<pattern-name>.png` into the output directory. Pattern images SHALL be looked up as `<pattern-path>/<pattern-name>.png` in the configured pattern paths and SHALL be scaled to the canvas size with nearest-neighbor filtering.

#### Scenario: All patterns rendered to PNG
- **WHEN** the tool runs the Cairo backend on a stylesheet with patterns "landuse_apiary" and "landuse_forest" and a pattern path containing the corresponding tiles
- **THEN** files `landuse_apiary.png` and `landuse_forest.png` exist in the output directory and each contains the scaled pattern tile

#### Scenario: Pattern image not found
- **WHEN** the tool runs the Cairo backend on a stylesheet with a pattern whose tile is not present in any configured pattern path
- **THEN** the tool prints an error message naming the pattern and exits with a non-zero exit code

#### Scenario: Pattern tiles are scaled faithfully
- **WHEN** the tool renders a pattern tile to the canvas size
- **THEN** the output image SHALL use nearest-neighbor filtering so the tile pixels are preserved

### Requirement: Pattern path resolution

The tool SHALL accept one or more `--pattern-path <dir>` options specifying directories to search for pattern images. When no `--pattern-path` is given, the tool SHALL fall back to the standard icon directory next to the stylesheet (`<stylesheet-dir>/../libosmscout/data/icons/14x14/standard`). When the stylesheet uses patterns but no pattern directory can be resolved, the tool SHALL print a warning and continue without rendering patterns.

#### Scenario: Explicit pattern path used
- **WHEN** the user passes `--pattern-path <dir>` with a directory containing pattern tiles
- **THEN** the tool searches that directory for pattern images

#### Scenario: Default pattern path used
- **WHEN** the user does not pass `--pattern-path` and the standard icon directory next to the stylesheet exists
- **THEN** the tool searches that directory for pattern images

#### Scenario: No pattern path found
- **WHEN** the stylesheet uses patterns but no pattern directory can be resolved
- **THEN** the tool prints a warning and exits with exit code zero

### Requirement: Pattern contact sheet output

The tool SHALL provide an optional contact-sheet mode that, in addition to individual pattern files, writes a single image containing a grid of all rendered patterns with their names, so many patterns can be scanned at once.

#### Scenario: Pattern contact sheet for Cairo backend
- **WHEN** the user enables contact-sheet mode with the Cairo backend on a stylesheet with several patterns
- **THEN** a single PNG file named `patterns.png` exists in the output directory showing every pattern in a grid with its name

### Requirement: List pattern names without rendering

The tool SHALL include pattern names in its list mode, so the full set of patterns can be printed without rendering or writing any image files.

#### Scenario: List mode includes patterns
- **WHEN** the user runs the tool in list mode on a stylesheet with symbols and patterns
- **THEN** the tool prints the symbol names followed by the pattern names (one per line) and exits with exit code zero
