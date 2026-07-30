# Font Management

**Purpose:** The Skia backend loads and caches `SkTypeface` instances keyed by font name. `GetFontHeight()` returns the actual font height from font metrics instead of a hardcoded value.

## Requirements

### Requirement: Font loading and caching

The Skia backend SHALL load and cache `SkTypeface` instances keyed by font name and size. `GetFontHeight()` SHALL return the actual font height from font metrics instead of a hardcoded value.

#### Scenario: GetFontHeight returns font metrics
- **WHEN** `GetFontHeight()` is called with a valid font name and size
- **THEN** it SHALL return the height based on `SkFontMetrics::fDescent - fAscent` or equivalent

#### Scenario: Fonts are cached by name and size
- **WHEN** the same font name and size are requested multiple times
- **THEN** the typeface SHALL be loaded from the system font manager only once

#### Scenario: Missing font falls back gracefully
- **WHEN** the requested font name is not available on the system
- **THEN** the backend SHALL fall back to a default typeface and log a warning
