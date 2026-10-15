# Spec Delta

## Purpose

Decide which of a style entry's two renderings a render draws when the entry carries both a raster icon
name and a vector symbol, and let a caller state the choice per render.

## ADDED Requirements

### Requirement: A two-rendering entry draws the preferred rendering

When a style entry carries both a raster icon name and a vector symbol, a render SHALL draw the entry's
vector symbol if the render prefers the symbol and the raster icon otherwise. The default SHALL be the
raster icon, so a render that does not set the preference keeps the present precedence.

#### Scenario: The default draws the raster icon

- **GIVEN** a style entry with both a raster icon name and a vector symbol
- **AND** a render that does not set the preference
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its raster icon
- **AND** it SHALL NOT register a symbol

#### Scenario: The preference draws the symbol

- **GIVEN** the same entry
- **AND** a render whose preference is the vector symbol
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its vector symbol

### Requirement: Preferring the symbol does not depend on the raster icon

With the vector symbol preferred, the entry SHALL register its symbol whether or not the raster icon image
exists or can be served. With the preference off, an entry whose raster icon cannot be served SHALL still
fall back to its symbol.

#### Scenario: Symbol preferred while the raster icon cannot be served

- **GIVEN** an entry with both renderings whose raster icon image cannot be served
- **AND** a render whose preference is the vector symbol
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its vector symbol

#### Scenario: Raster icon unavailable falls back to the symbol

- **GIVEN** an entry with both renderings whose raster icon image cannot be served
- **AND** a render that does not set the preference
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its vector symbol

#### Scenario: Symbol preferred while the raster icon exists

- **GIVEN** an entry with both renderings whose raster icon image can be served
- **AND** a render whose preference is the vector symbol
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its vector symbol
- **AND** it SHALL NOT register its raster icon

### Requirement: An entry with one rendering is unaffected by the preference

An entry that carries only a raster icon or only a vector symbol SHALL register that rendering regardless
of the preference. An entry whose only rendering cannot be served SHALL register no label.

#### Scenario: Symbol-only entry under the default

- **GIVEN** an entry that carries a vector symbol and no raster icon name
- **AND** a render that does not set the preference
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register its vector symbol

#### Scenario: Icon-only entry whose image cannot be served

- **GIVEN** an entry that carries a raster icon name and no vector symbol
- **AND** its raster icon image cannot be served
- **WHEN** the frame is prepared
- **THEN** the entry SHALL register no label

### Requirement: The preference takes effect on the next frame

A preference set after a stylesheet was loaded SHALL apply to the next frame that is prepared, SHALL NOT
require a stylesheet reload, and SHALL NOT change the rendering of a frame prepared before it.

#### Scenario: The next frame follows the new preference

- **GIVEN** a loaded stylesheet and a render whose preference is the raster icon
- **WHEN** the preference is changed to the vector symbol and the next frame is prepared
- **THEN** a two-rendering entry of that frame SHALL register its vector symbol
- **AND** the stylesheet SHALL NOT have been reloaded

#### Scenario: An earlier frame is unchanged

- **GIVEN** a frame already prepared with the raster icon preferred
- **WHEN** the preference is changed afterwards
- **THEN** the labels of the prepared frame SHALL still name the raster icon
