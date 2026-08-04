# Basemap UI Specification

## Purpose

Show basemap status in the JavaScout UI, including installation state, download/update controls, and visual indication that the basemap is active.

## Requirements

### Requirement: Show basemap in installed maps list

The system SHALL display the basemap as an entry in the installed maps list within the Map Download dialog.

#### Scenario: Basemap installed
- **WHEN** user opens Map Download dialog
- **WHEN** basemap is installed
- **THEN** installed maps list SHALL show "World Basemap" entry
- **THEN** entry SHALL show basemap size and version

#### Scenario: Basemap not installed
- **WHEN** user opens Map Download dialog
- **WHEN** basemap is not installed
- **THEN** installed maps list SHALL NOT show a basemap entry

### Requirement: Show basemap download/update button

The system SHALL provide a button or control to download or update the basemap in the Map Download dialog.

#### Scenario: Download basemap
- **WHEN** basemap is available on server but not installed
- **THEN** Map Download dialog SHALL show a "Download Basemap" button
- **WHEN** user clicks "Download Basemap"
- **THEN** system starts basemap download with progress indication

#### Scenario: Update basemap
- **WHEN** basemap is installed and a newer version is available
- **THEN** Map Download dialog SHALL show an "Update Basemap" button
- **WHEN** user clicks "Update Basemap"
- **THEN** system starts basemap update with progress indication

### Requirement: Show basemap status in main view

The system SHALL indicate basemap status in the main map view (e.g., in the status bar or as a subtle indicator).

#### Scenario: Basemap active
- **WHEN** basemap is loaded and rendering
- **THEN** status bar SHALL show a basemap indicator (e.g., "Basemap: active")

#### Scenario: No basemap
- **WHEN** no basemap is installed
- **THEN** status bar SHALL NOT show a basemap indicator
