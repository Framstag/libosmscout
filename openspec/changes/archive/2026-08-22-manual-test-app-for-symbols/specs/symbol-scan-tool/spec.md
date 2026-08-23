## Purpose

Lets a developer render every symbol defined by a stylesheet to standalone image files, so symbol definitions (including AI-generated ones) can be visually scanned for correctness.

## ADDED Requirements

### Requirement: Symbol enumeration API
The library SHALL expose the names of all symbols defined by a loaded stylesheet, so a client can iterate over the full set of symbols without knowing them in advance.

#### Scenario: Enumerate symbols of a loaded stylesheet
- **WHEN** a stylesheet defining symbols "place_city" and "place_town" has been loaded
- **THEN** the client can retrieve the complete list of symbol names, containing exactly "place_city" and "place_town"

#### Scenario: Stylesheet without symbols
- **WHEN** a stylesheet defining no symbols has been loaded
- **THEN** the enumeration returns an empty list

### Requirement: Load stylesheet for symbol scanning
The tool SHALL load the stylesheet given on the command line together with a type definition file, and SHALL report a load failure with a non-zero exit code when either file is missing or invalid.

#### Scenario: Stylesheet loads successfully
- **WHEN** the user invokes the tool with a valid stylesheet file and a valid type definition file
- **THEN** the tool proceeds with scanning the symbols of that stylesheet

#### Scenario: Stylesheet does not exist
- **WHEN** the user invokes the tool with a stylesheet path that does not exist
- **THEN** the tool prints an error message and exits with a non-zero exit code

#### Scenario: Type definition file resolution
- **WHEN** the user does not pass an explicit type definition file
- **THEN** the tool uses a `map.ost` next to the stylesheet, falling back to a type definition file with the stylesheet's name in the same directory, and fails with a non-zero exit code if none can be found

### Requirement: Render all symbols via Cairo backend
The tool SHALL render every symbol of the loaded stylesheet standalone and centered on a plain background using the Cairo backend, and SHALL write one PNG file per symbol named `<symbol-name>.png` into the output directory.

#### Scenario: All symbols rendered to PNG
- **WHEN** the tool runs the Cairo backend on a stylesheet with symbols "place_city" and "place_town" and an existing output directory
- **THEN** files `place_city.png` and `place_town.png` exist in the output directory and each contains the rendered symbol

#### Scenario: Output directory is created
- **WHEN** the user specifies an output directory that does not exist
- **THEN** the tool creates the directory (including parent directories) before writing images

#### Scenario: Cairo backend unavailable
- **WHEN** the tool is built without Cairo support and the user selects the Cairo backend
- **THEN** the tool prints an error message and exits with a non-zero exit code

### Requirement: Render all symbols to SVG
The CLI SHALL render each symbol of the loaded stylesheet using the SVG backend, writing a well-formed SVG file per symbol to `<symbol-name>.svg` into the output directory, when the SVG backend is selected.

#### Scenario: All symbols rendered to SVG files
- **WHEN** the tool runs the SVG backend on a stylesheet with symbols "place_city" and "place_town"
- **THEN** files `place_city.svg` and `place_town.svg` exist in the output directory, each containing a valid SVG document with the symbol drawing primitives

### Requirement: Render all symbols in a single run
The tool SHALL support selecting one or both backends, and SHALL process every symbol of the stylesheet in a single invocation for each selected backend.

#### Scenario: All backends selected
- **WHEN** the user selects both Cairo and SVG backends on a stylesheet with two symbols
- **THEN** four image files are produced: two PNG and two SVG files

#### Scenario: Single backend selected
- **WHEN** the user selects only the SVG backend
- **THEN** only SVG files are produced and no PNG files are written

### Requirement: List symbol names without rendering
The tool SHALL provide a mode that prints the name of every symbol of the loaded stylesheet without rendering or writing any image files.

#### Scenario: List mode
- **WHEN** the user runs the tool in list mode on a stylesheet with symbols "place_city" and "place_town"
- **THEN** the tool prints exactly the names "place_city" and "place_town" (one per line) and exits with exit code zero

### Requirement: Contact sheet output
The tool SHALL provide an optional contact-sheet mode that, instead of or in addition to individual files, writes a single image per backend containing a grid of all rendered symbols with their names, so many symbols can be scanned at once.

#### Scenario: Contact sheet for Cairo backend
- **WHEN** the user enables contact-sheet mode with the Cairo backend on a stylesheet with several symbols
- **THEN** a single PNG file exists in the output directory showing every symbol in a grid with its name

#### Scenario: Contact sheet for SVG backend
- **WHEN** the user enables contact sheet mode with the SVG backend
- **THEN** a single SVG file exists in the output directory showing every symbol in a grid with its name

### Requirement: Failure reporting
The tool SHALL report per-symbol rendering or file-writing failures and SHALL exit with a non-zero exit code if any requested render or write failed.

#### Scenario: Symbol render fails
- **WHEN** a symbol cannot be rendered or its output image cannot be written
- **THEN** the tool prints an error message naming the symbol and exits with a non-zero exit code

#### Scenario: All symbols succeed
- **WHEN** all selected backends render and write every symbol successfully
- **THEN** the tool prints a success summary and exits with exit code zero
