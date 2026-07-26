## What Changes

Add location search capability to JavaScout app. Users can search for locations via a map overlay UI. The search triggers a new API endpoint on the clientjavajar backend. Results display sorted, and clicking a result navigates the map to that location.

## Capabilities

### New Capabilities

- `location-search-ui`: Map overlay search button that expands into a search text field with result list. Full-width list on small screens, popup on large screens. Cancel button returns to map overview.
- `location-search-api`: New API in clientjavajar for location search queries, returning sorted results with coordinates.
- `location-search-navigation`: Clicking a search result moves the map view to the selected location.

### Modified Capabilities

None.

## Impact

- **clientjavajar**: New API endpoint for location search (query → sorted results with geo coords)
- **JavaScout UI**: Overlay search button, expandable search bar, result list rendering
- **Responsive layout**: Different result list presentation for small vs large screens
- **Map navigation**: Pan/zoom to selected location on result click
