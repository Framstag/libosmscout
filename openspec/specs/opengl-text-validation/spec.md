# OpenGL Text Validation

## Purpose

Ensure DrawMapOpenGL CLI validates font-related parameters with clear errors before invoking the render pipeline.

## Requirements

### Requirement: Font path validated before painter construction
DrawMapOpenGL SHALL validate that `--fontName` points to an existing, readable regular file before constructing `MapPainterOpenGL`. If validation fails, a non-zero exit code and clear error message SHALL be produced.

#### Scenario: Valid font path accepted
- **WHEN** user provides `--fontName /path/to/valid/font.ttf` and the file exists and is readable
- **THEN** application proceeds without font-path error

#### Scenario: Non-existent font path rejected
- **WHEN** user provides `--fontName /path/to/nonexistent.ttf`
- **THEN** application prints error message containing "Font file" and "does not exist" and exits with error code 1

#### Scenario: Directory path rejected
- **WHEN** user provides `--fontName /usr/share/fonts/` (a directory, not a file)
- **THEN** application prints error message and exits with error code 1

#### Scenario: Empty font path rejected
- **WHEN** user provides `--fontName ""` (empty string)
- **THEN** application prints error message and exits with error code 1

### Requirement: Font size validated
DrawMapOpenGL SHALL validate that `--fontSize` is positive. If zero or negative, a non-zero exit code and clear error message SHALL be produced.

#### Scenario: Zero font size rejected
- **WHEN** user provides `--fontSize 0`
- **THEN** application prints error message about invalid font size and exits with error code 1

#### Scenario: Negative font size rejected
- **WHEN** user provides `--fontSize -1`
- **THEN** application prints error message about invalid font size and exits with error code 1

#### Scenario: Reasonable font size accepted
- **WHEN** user provides `--fontSize 5` (or any positive value)
- **THEN** application proceeds without font-size error

### Requirement: TextLoader init failure produces clear error
If `TextLoader` fails to initialize (font face load fails), `MapPainterOpenGL` SHALL log the error with `log.Error()` including the font path. DrawMapOpenGL SHALL check `painter->IsInitialized()` and exit with code 1 on failure.

#### Scenario: Corrupted font file
- **WHEN** user provides `--fontName /path/to/corrupted.ttf` (file exists but FreeType rejects it)
- **THEN** `TextLoader::IsInitialized()` returns false, `log.Error()` message printed, application exits with code 1

#### Scenario: Valid font, cleanup on early init failure
- **WHEN** font loads correctly but shader compilation fails
- **THEN** `MapPainterOpenGL::IsInitialized()` returns false, error logged, application exits with code 1
