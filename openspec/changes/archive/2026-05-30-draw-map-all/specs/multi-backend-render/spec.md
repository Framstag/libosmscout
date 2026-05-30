## ADDED Requirements

### Requirement: Unified command-line interface

The system SHALL provide a single executable `DrawMapAll` that accepts the same positional and optional arguments as existing DrawMap applications.

#### Scenario: Accepts standard DrawMap arguments

- **WHEN** user invokes `DrawMapAll <stylesheet> <lat> <lon> <zoom> <outputDir>`
- **THEN** the application parses the same arguments as other DrawMap apps, with `outputDir` replacing the `output` positional argument

#### Scenario: Accepts optional rendering flags

- **WHEN** user passes `--dpi`, `--angle`, `--width`, `--height`, `--fontName`, `--fontSize`, `--iconMode`, `--iconPath`, `--hillshading`, `--contourlines`, `--database`, `--baseMap`, `--srtmDirectory`, `--debug`
- **THEN** all flags are applied identically as in existing DrawMap applications

### Requirement: Output directory creation

The system SHALL create the output directory if it does not exist.

#### Scenario: Directory already exists

- **WHEN** the specified output directory already exists
- **THEN** the application writes output files into it without error

#### Scenario: Directory does not exist

- **WHEN** the specified output directory does not exist
- **THEN** the application creates it (including parent directories) and writes output files into it

### Requirement: Per-backend output files

The system SHALL write one output file per compiled-in backend to the output directory.

#### Scenario: Standard file naming

- **WHEN** backends AGG, Cairo, OpenGL, Qt, SVG, OSX, DirectX, GDI are compiled in
- **THEN** output files are named `DrawMapAgg.png`, `DrawMapCairo.png`, `DrawMapOpenGL.png`, `DrawMapQt.png`, `DrawMapSVG.svg`, `DrawMapOSX.png`, `DrawMapDirectX.png`, `DrawMapGDI.png` respectively

#### Scenario: Raster backends produce PNG

- **WHEN** a raster backend (AGG, Cairo, OpenGL, Qt, OSX, DirectX, GDI) renders successfully
- **THEN** the output file is a valid PNG image of the specified width and height

#### Scenario: SVG backend produces SVG

- **WHEN** the SVG backend renders successfully
- **THEN** the output file is a valid SVG document with `.svg` extension

### Requirement: Build-time backend detection

The system SHALL detect available rendering backends at build time and only compile the corresponding code blocks.

#### Scenario: Backend headers available

- **WHEN** `OSMSCOUT_BUILD_MAP_AGG` is enabled in CMake
- **THEN** the AGG rendering block is compiled with `HAVE_OSMSCOUT_MAP_AGG` defined

#### Scenario: Backend headers unavailable

- **WHEN** a backend library is not found or its build option is OFF
- **THEN** the corresponding code block is excluded from compilation; no error

### Requirement: Error handling per backend

The system SHALL handle rendering failures per-backend without aborting other backends.

#### Scenario: Single backend fails

- **WHEN** one backend fails to initialize or render
- **THEN** an error message is printed to stderr and execution continues with remaining backends

#### Scenario: All backends fail

- **WHEN** all backends fail to render
- **THEN** the application exits with a non-zero exit code

### Requirement: AGG RGB to PNG conversion

The system SHALL convert AGG's raw RGB pixel buffer to PNG using libpng.

#### Scenario: AGG renders to buffer

- **WHEN** `MapPainterAgg::DrawMap()` returns successfully
- **THEN** the packed RGB24 buffer is written to `DrawMapAgg.png` via libpng

### Requirement: Cairo native PNG output

The system SHALL use Cairo's native PNG writer when Cairo is available.

#### Scenario: Cairo renders to surface

- **WHEN** `MapPainterCairo::DrawMap()` returns successfully
- **THEN** `cairo_surface_write_to_png()` saves `DrawMapCairo.png`

### Requirement: OpenGL offscreen rendering to PNG

The system SHALL create a hidden GLFW window, render via OpenGL, and read back pixels via `glReadPixels`.

#### Scenario: OpenGL renders offscreen

- **WHEN** `MapPainterOpenGL::ProcessData()` and `DrawMap()` return successfully
- **THEN** the Y-flipped RGB buffer is written to `DrawMapOpenGL.png` via libpng

### Requirement: Qt pixmap to PNG

The system SHALL use Qt's native PNG writer when Qt is available.

#### Scenario: Qt renders to QPixmap

- **WHEN** `MapPainterQt::DrawMap()` returns successfully
- **THEN** `QPixmap::save("PNG")` saves `DrawMapQt.png`

### Requirement: SVG vector output

The system SHALL write SVG data to a file without rasterization.

#### Scenario: SVG renders to stream

- **WHEN** `MapPainterSVG::DrawMap()` returns successfully
- **THEN** the SVG stream is written to `DrawMapSVG.svg`

### Requirement: DirectX offscreen WIC bitmap to PNG (Windows only)

The system SHALL create an offscreen Direct2D render target backed by a WIC bitmap for PNG output.

#### Scenario: DirectX renders to WIC bitmap

- **WHEN** `MapPainterDirectX::DrawMap()` renders to a WIC bitmap render target
- **THEN** the WIC bitmap encoder saves `DrawMapDirectX.png`

### Requirement: GDI offscreen DIBSection to PNG (Windows only)

The system SHALL create an offscreen GDI device context backed by a DIBSection for PNG output.

#### Scenario: GDI renders to DIBSection

- **WHEN** `MapPainterGDI::DrawMap()` renders to a DIBSection HDC
- **THEN** the bitmap is saved as `DrawMapGDI.png` via GDI+

### Requirement: OSX CoreGraphics to PNG (macOS only)

The system SHALL create a CoreGraphics bitmap context and render via MapPainterIOS.

#### Scenario: OSX renders to CGContext

- **WHEN** `MapPainterIOS::DrawMap()` renders to a CGBitmapContext
- **THEN** the bitmap context is written to `DrawMapOSX.png` via NSBitmapImageRep
