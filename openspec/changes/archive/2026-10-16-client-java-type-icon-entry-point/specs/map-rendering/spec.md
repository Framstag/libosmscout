# Spec Delta

## Purpose

Provide the Java client's rendering surface: a map frame from a position and a magnification, the same frame with route, POI markers and track overlay, the projection helper, and the small renderings a UI needs besides a frame.

## ADDED Requirements

### Requirement: JNI entry point returns a type's stylesheet artwork

The JNI bridge SHALL offer `renderTypeIcon(String objectType, int sizePx)` (Java `public native int[]`) returning an `int[]` of `sizePx * sizePx` pixels in the layout the allocating render entry point returns, or `null` when the type has no artwork or the artwork cannot be produced. It SHALL resolve the type through the loaded type configuration and its icon style through the style configuration's own selectors, and SHALL need no map frame, viewport or projection of the caller's.

#### Scenario: A stylesheet icon is rendered

- **WHEN** `renderTypeIcon("amenity_parking", 48)` is called on a client with an open database whose active stylesheet declares an icon for that type
- **THEN** the result SHALL have `48 * 48` elements
- **AND** the artwork SHALL be centred in the square and scaled to fit it

#### Scenario: A type without artwork reports nothing

- **WHEN** a type the stylesheet gives no icon at all is requested (a street, an address)
- **THEN** the entry point SHALL return `null`
- **AND** it SHALL NOT raise an error

#### Scenario: A type-dependent rule needs an object and falls back

- **WHEN** the stylesheet's rule for the type selects on a tag value rather than on the type alone (a religion, a brand)
- **THEN** the entry point SHALL report no artwork rather than an invented icon, because it loads no object
- **AND** it SHALL NOT raise an error

#### Scenario: An unusable raster icon falls back to the symbol

- **WHEN** the stylesheet's icon is an image the renderer cannot load (a stylesheet may name artwork that ships as SVG only)
- **THEN** the entry point SHALL draw the entry's vector symbol when it has one
- **AND** SHALL report no artwork only when the entry has neither

### Requirement: The type icon entry point follows the icon-versus-symbol preference

When the client prefers a style entry's vector symbol over its raster icon (`setPreferSymbolIcons(true)`) and the entry carries a symbol, the entry point SHALL draw that symbol. Otherwise it SHALL draw in the map's own order — the raster icon first, the symbol when the image is unusable.

#### Scenario: The preference draws the symbol

- **WHEN** the client's icon-versus-symbol preference is on and the entry carries both a symbol and an image
- **THEN** the rendered artwork SHALL be the symbol

#### Scenario: Without the preference the image comes first

- **WHEN** the preference is off and the entry carries both
- **THEN** the entry point SHALL draw the image, and the symbol only if the image is unusable

#### Scenario: The preference does not disable a symbol-only entry

- **WHEN** the preference is off and the entry carries a symbol but no image
- **THEN** the entry point SHALL draw the symbol

### Requirement: The type icon entry point bounds the request it accepts

`renderTypeIcon` SHALL return `null` for a `sizePx` that is not positive or is larger than 512, for a type name that is empty or unknown to the loaded type configuration, and for a client without a loaded database. It SHALL NOT fault, and `null` SHALL be the only failure a caller observes.

#### Scenario: A size outside the accepted range

- **WHEN** `renderTypeIcon(type, 0)`, `renderTypeIcon(type, -1)` or `renderTypeIcon(type, 513)` is called
- **THEN** the entry point SHALL return `null`
- **AND** it SHALL NOT fault

#### Scenario: An unknown or empty type name

- **WHEN** the requested type is not in the loaded type configuration, or the name is empty
- **THEN** the entry point SHALL return `null`

#### Scenario: No database is loaded

- **WHEN** the client has no open database or has not been initialised
- **THEN** the entry point SHALL return `null`
- **AND** it SHALL NOT fault

#### Scenario: The Cairo backend is not available

- **WHEN** the bridge was built without the Cairo map backend
- **THEN** the entry point SHALL return `null` and warn once
- **AND** it SHALL NOT fault

### Requirement: The type icon entry point's pixels are straight ARGB

One element per pixel SHALL be the word `0xAARRGGBB` with straight (non-premultiplied) channels, the layout the allocating render entry point returns, although the backend's surface stores premultiplied channels in its own byte order. A pixel the stylesheet leaves unpainted SHALL be fully transparent.

#### Scenario: The layout is the allocating entry point's

- **WHEN** the returned elements are inspected
- **THEN** each SHALL be one `0xAARRGGBB` word, index `y * sizePx + x` for the pixel at `(x, y)`

#### Scenario: An unpainted pixel is fully transparent

- **WHEN** the type's artwork does not cover a pixel of the square
- **THEN** that element SHALL be `0x00000000`

#### Scenario: An anti-aliased edge is un-premultiplied

- **WHEN** the artwork has a partially transparent edge pixel
- **THEN** its channels SHALL be the original colour, not the colour darkened by its own alpha

### Requirement: The type icon entry point reports why it produced no artwork

A request the entry point cannot satisfy SHALL be logged with the stage that decided it, the type and the requested size, so a missing icon is diagnosable. A type the stylesheet simply gives no icon is the normal case and SHALL NOT be logged as an error.

#### Scenario: A failure names its stage

- **WHEN** the entry point reports no artwork because no database carries a usable style configuration, the type is unknown, or the artwork could not be drawn
- **THEN** a warning SHALL name that stage together with the type and the size

#### Scenario: A type without artwork is silent

- **WHEN** a type is known and its style entry is simply not visible
- **THEN** no warning SHALL be logged for it
