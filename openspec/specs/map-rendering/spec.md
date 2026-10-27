## Purpose

Render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI. The rendering pipeline loads tile data from the database, renders with `MapPainterCairo`, and blits the pixel buffer to a JavaFX Canvas.

## Requirements

### Requirement: Render map to JavaFX Canvas
The system SHALL render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI.

#### Scenario: Initial render after database load
- **WHEN** database finishes loading
- **THEN** system calls `OSMScoutClient.render(w, h, lat, lon, mag)` via JNI
- **THEN** system receives `int[]` ARGB pixel data
- **THEN** system writes pixels to Canvas via `PixelWriter.setPixels()`
- **THEN** map is visible in the center panel

#### Scenario: Re-render on view change
- **WHEN** user pans or zooms the map
- **THEN** system calls `render()` with new lat/lon/mag after 200ms debounce
- **THEN** Canvas updates with new rendered frame

#### Scenario: Render with no database
- **WHEN** no database is loaded
- **THEN** Canvas shows empty/background color
- **THEN** no JNI render call is made

### Requirement: Renderer is initialised with a valid icon directory
The system SHALL ensure the renderer is initialised with a valid icon directory so POI icons can be drawn.

#### Scenario: Renderer has icon paths configured
- **WHEN** the native client is built with a non-empty icon directory
- **THEN** `MapParameter::SetIconPaths()` SHALL be called before rendering
- **THEN** `MapPainterCairo` SHALL use the configured directory to locate icons

### Requirement: Renderer receives the configured icon directory
The system SHALL pass the configured icon directory to the Cairo renderer via `MapParameter::SetIconPaths()`.

#### Scenario: Icon directory reaches the renderer
- **WHEN** the native client is built with a non-empty icon directory
- **THEN** `OSMScoutClient.cpp` SHALL call `params.SetIconPaths({iconDir})` before `MapPainterCairo::DrawMap()`
- **THEN** the renderer SHALL search for icons in that directory

#### Scenario: POI icons rendered when icon directory is configured
- **WHEN** the native client is built with a non-empty icon directory pointing to PNG icons
- **THEN** JavaScout SHALL append a trailing path separator to the directory before passing it to the builder
- **THEN** the renderer SHALL load `.png` icons from that directory
- **THEN** POI icons SHALL appear on the rendered map

#### Scenario: No icons rendered when icon directory is empty
- **WHEN** the native client is built without an icon directory
- **THEN** the renderer SHALL not attempt to load icons
- **THEN** the map SHALL render without POI icons

#### Scenario: SVG icons are not loaded by the Cairo renderer
- **WHEN** the configured icon directory contains only SVG files
- **THEN** the Cairo renderer SHALL fail to load those icons
- **THEN** an error message SHALL be logged for each missing PNG icon

### Requirement: JNI render method
`OSMScoutClient` SHALL expose a native `render()` method that renders the current map view to an ARGB pixel
array. The request SHALL state the physical DPI of the display the frame is rendered for, and SHALL project
with that value. A request that states no usable DPI SHALL project with the DPI configured on the client,
and the request SHALL NOT change that configured value.

#### Scenario: Render returns pixel data
- **WHEN** `render(width, height, lat, lon, angle, magnification, dpi)` is called
- **THEN** C++ creates `MercatorProjection` with the given center, angle, magnification and DPI
- **THEN** C++ loads tile data from databases via `MapService`
- **THEN** C++ renders via `MapPainterCairo::DrawMap()` to Cairo image surface
- **THEN** C++ converts BGRx to `int[]` ARGB
- **THEN** Java receives non-null `int[]` of length `width * height`

#### Scenario: Render with invalid parameters
- **WHEN** `render()` is called with width=0 or height=0
- **THEN** method returns `null`
- **WHEN** `render()` is called before database is initialized
- **THEN** method returns `null`

#### Scenario: The request's DPI scales the frame
- **GIVEN** two requests that differ only in the DPI they state
- **WHEN** both frames are rendered for the same center, angle and magnification
- **THEN** the frames SHALL be projected with their respective DPI values
- **AND** the labels and icons of the frame rendered at the higher DPI SHALL be smaller in geographic extent

#### Scenario: A request without a usable DPI uses the configured one
- **GIVEN** a client whose configured render DPI is known
- **WHEN** a request states an undefined or non-positive DPI
- **THEN** the frame SHALL be projected with the client's configured DPI
- **AND** the request SHALL NOT change the client's configured DPI
- **AND** the call SHALL NOT be rejected

#### Scenario: One surface's request does not change another surface's frame
- **GIVEN** a client used from two surfaces with different display densities
- **WHEN** each surface states its own DPI in its render request
- **THEN** each frame SHALL be projected with the DPI its own request stated
- **AND** neither request SHALL change the DPI the other frame is rendered with

### Requirement: JNI render method supports optional route, POI markers, and track overlay

The system SHALL expose a native render method that renders the current map view with optional route polyline, favorite/search marker POIs, and imported track polyline. The current-location marker is no longer rendered by the native backend. The request SHALL state the physical DPI of the display the frame is rendered for, under the same rule as the base render method.

#### Scenario: Render with route, markers, and track

- **WHEN** `renderWithRouteAndPois(width, height, lat, lon, angle, mag, dpi, routeLats, routeLons, favoriteLats, favoriteLons, searchSelLat, searchSelLon, trackLats, trackLons)` is called
- **THEN** C++ SHALL create a synthetic `_route` WAY from route coordinates when provided
- **AND** C++ SHALL create synthetic `_favorite`, `_search_selected`, `_route_start`, and `_route_end` NODEs when their coordinates are provided
- **AND** C++ SHALL create a synthetic `_track` WAY from track coordinates when provided
- **AND** C++ SHALL add all synthetic objects to `MapData` before `MapPainterCairo::DrawMap()`
- **AND** C++ SHALL project with the DPI the request stated
- **AND** Java SHALL receive non-null `int[]` of length `width * height`

#### Scenario: Render without track overlay

- **GIVEN** `trackLats` and `trackLons` are null or empty
- **WHEN** the render method is called
- **THEN** no `_track` WAY SHALL be added to `MapData`
- **AND** the map SHALL render without a track overlay

#### Scenario: Render without current location overlay

- **GIVEN** current location rendering is handled by the JavaFX overlay layer
- **WHEN** `renderWithRouteAndPois()` is called without current-location parameters
- **THEN** C++ SHALL not add any synthetic current-location node to `MapData`

#### Scenario: The convenience form states no DPI
- **GIVEN** a caller that uses the render method without a DPI argument
- **WHEN** the call is made
- **THEN** it SHALL forward the request with no DPI stated
- **AND** the frame SHALL be projected with the client's configured DPI

### Requirement: Renderer can draw current-location marker during navigation
The system SHALL render a current-location marker on the map when a navigation position estimate is active.

#### Scenario: Render with current location
- **WHEN** a navigation position is available and the map is rendered
- **THEN** JavaScout paints the marker on top of the Canvas after the base map image is blitted
- **AND** the marker is always drawn last, so it appears above roads, POIs, route and track overlays

### Requirement: Renderer can draw heading indicator during navigation
The system SHALL render a heading/accuracy indicator for the current location when heading and accuracy information are available.

#### Scenario: Render heading indicator
- **WHEN** a bearing and horizontal accuracy are available
- **THEN** JavaScout draws a direction arrow aligned with the bearing
- **AND** marker tint reflects GPS quality (blue=unknown, green=good, yellow=moderate, red=poor)

### Requirement: JNI exposes map projection helper
The system SHALL expose a native helper that projects a geographic coordinate to viewport pixel coordinates for a given map view, so Java-side overlays can be positioned without reimplementing the projection.

#### Scenario: Project geographic coordinate to pixel
- **WHEN** `projectToPixel(width, height, centerLat, centerLon, magnification, dpi, lat, lon)` is called
- **THEN** C++ SHALL create the same `MercatorProjection` used for rendering
- **AND** C++ SHALL return `double[]{x, y}` in viewport pixel coordinates, or null if invalid/outside

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

### Requirement: The render states the label line-wrapping bounds

The JNI render SHALL state all four label line-wrapping values on its rendering parameter instead of
inheriting them: a minimum character count of 5, a maximum of 15, fit-to-area enabled, and fit-to-width the
shorter side of the render buffer, that buffer carrying the pan overrun. The stated fit-to-width SHALL NOT
be the library default of 8000 px, which does not bind in a viewport of any phone or desktop size.

#### Scenario: A long multi-word label is wrapped

- **GIVEN** a viewport whose shorter side is well below the unwrapped width of a long multi-word label in
  the map style
- **WHEN** a frame is rendered
- **THEN** the label SHALL be drawn on more than one line
- **AND** no line SHALL be wider than the shorter side of the buffer

#### Scenario: A one-word label stays on one line

- **GIVEN** a label that is a single word, however long
- **WHEN** a frame is rendered
- **THEN** it SHALL be drawn on one line, because the layouter breaks only at word boundaries

#### Scenario: A short label is not wrapped

- **GIVEN** a label shorter than the minimum character count
- **WHEN** a frame is rendered
- **THEN** it SHALL be drawn on one line

#### Scenario: The bound follows the buffer the caller asked for

- **GIVEN** two render requests that differ only in viewport size
- **WHEN** both frames are rendered
- **THEN** each SHALL wrap against the shorter side of its own buffer
- **AND** the wider request SHALL allow a wider line

### Requirement: The stated bounds are a decision a host can read

The four bounds SHALL be produced by one dependency-free function of the buffer size that the render uses
and a host test can call, so the applied values cannot drift from the stated ones. The function SHALL take
the shorter side: the width for a tall buffer, the height for a wide one.

#### Scenario: A tall buffer bounds by its width

- **GIVEN** a buffer 360 wide and 640 high
- **WHEN** the decision is read
- **THEN** fit-to-width SHALL be 360
- **AND** the character bounds SHALL be 5 and 15 and fit-to-area SHALL be enabled

#### Scenario: A wide buffer bounds by its height

- **GIVEN** a buffer 1024 wide and 768 high
- **WHEN** the decision is read
- **THEN** fit-to-width SHALL be 768

#### Scenario: The decision is not the non-binding library default

- **GIVEN** a buffer of the sizes above
- **WHEN** the decision's fit-to-width is compared with the library default of an unconfigured parameter
- **THEN** it SHALL differ from 8000 px in every case
