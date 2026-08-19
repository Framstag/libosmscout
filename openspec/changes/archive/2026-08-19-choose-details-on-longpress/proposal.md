## Why

Long-press currently auto-selects exactly one candidate object for showing its description. When several objects overlap at the pressed coordinate (e.g. street, building, and POI), the auto-selected object is often not the one the user meant. The user has no way to pick a different object without repeating the gesture elsewhere.

## What Changes

- Long-press on the map shows a list of reasonable candidate objects instead of directly showing a single description
- Each candidate entry is presented with the same description format used in search results
- The user selects one entry from the list to view its details
- Selecting an entry opens the existing description overlay dialog for that object
- When exactly one candidate exists, the details are shown directly without an intermediate list
- When no candidates exist, the existing "No description available" behavior remains

## Capabilities

### New Capabilities

- `long-press-candidate-picker`: Presents the list of objects found at a long-pressed map coordinate, formatted like search results, and lets the user choose which object's details to view

### Modified Capabilities

- `long-press-description`: The long-press flow no longer auto-selects a single object. It now produces a candidate list for user selection (or direct display when only one candidate exists)

## Impact

- JavaScout client application: long-press gesture handling and description display flow
- Description retrieval by coordinate: now must return all reasonable candidates, not just the top-ranked one
- Overlay dialog / candidate list UI: new list presentation reusing search-result description formatting
- No changes to the core libosmscout description services
