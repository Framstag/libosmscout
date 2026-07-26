# javascout-gpx-track-import

## Purpose

Allow JavaScout to import track points from a GPX file through the Java client JNI bridge.

## ADDED Requirements

### Requirement: Java API exposes GPX track import

The system SHALL provide a Java method on `OSMScoutClient` that reads the first track from a GPX file and returns its points as an array.

#### Scenario: Import valid GPX file

- **GIVEN** a GPX file containing at least one track with at least one segment
- **WHEN** `OSMScoutClient.importGpxTrack(filePath)` is called
- **THEN** it SHALL return a non-empty `TrackPoint[]`
- **AND** each `TrackPoint` SHALL contain latitude, longitude, and optional timestamp

#### Scenario: Import empty GPX file

- **GIVEN** a GPX file with no tracks
- **WHEN** `OSMScoutClient.importGpxTrack(filePath)` is called
- **THEN** it SHALL return an empty `TrackPoint[]`

#### Scenario: Import non-existent file

- **GIVEN** a file path that does not exist
- **WHEN** `OSMScoutClient.importGpxTrack(filePath)` is called
- **THEN** it SHALL return an empty `TrackPoint[]`
- **AND** it SHALL log an error

### Requirement: TrackPoint data class models a single point

The system SHALL provide a Java class `com.framstag.libosmscout.client.TrackPoint` with public fields for latitude, longitude, and timestamp.

#### Scenario: TrackPoint stores coordinates

- **WHEN** a `TrackPoint` is constructed with latitude, longitude, and timestamp
- **THEN** the fields SHALL be readable via public access

### Requirement: JNI imports via libosmscout-gpx

The native implementation of `importGpxTrack` SHALL call `osmscout::gpx::ImportGpx` and convert the resulting `GpxFile` track points into Java `TrackPoint` objects.

#### Scenario: JNI reads first track only

- **GIVEN** a GPX file with multiple tracks
- **WHEN** `importGpxTrack` is invoked
- **THEN** only points from the first track SHALL be returned
- **AND** all segments of that first track SHALL be concatenated in order

### Requirement: Import feature is gated by GPX build option

The system SHALL only expose `importGpxTrack` when the build includes `libosmscout-gpx`. When GPX support is disabled, the method SHALL return an empty array and log a warning.

#### Scenario: GPX support disabled

- **GIVEN** a build with `OSMSCOUT_BUILD_GPX=OFF`
- **WHEN** `importGpxTrack` is called
- **THEN** it SHALL return an empty `TrackPoint[]`
- **AND** it SHALL log that GPX support is disabled
