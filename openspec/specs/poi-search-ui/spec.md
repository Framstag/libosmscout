# poi-search-ui Specification

## Purpose

Describe the POI search UI in JavaScout: opening the search from the main menu, choosing a POI category, defining the search area, and browsing results with details on long-click.

## Requirements

### Requirement: POI category selection

The POI search UI SHALL let the user choose a POI category from a list.

#### Scenario: Category list shows supported categories
- **WHEN** the POI search UI opens
- **THEN** the category list SHALL show the categories: Hotels, Restaurants, Grocery store, Viewpoint, Museum, Gas station, Charging station, ATM, Tourism, Parking, Police station, Hospital, Doctors office, Public transport

#### Scenario: Category selection drives search
- **WHEN** the user selects a category and triggers the search
- **THEN** the search SHALL use the selected category
- **AND** results SHALL contain only POIs of that category
