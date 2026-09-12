# Web Server Specification

## Purpose

Containerized read-only web server that exposes exactly the public part of the map repository — the region index and the version slots — over plain HTTP without any authentication, and an orchestration file that runs it together with the mapgen regeneration container on one shared repository volume.

## Requirements

### Requirement: Served surface
The web server image SHALL serve only the public part of the repository volume (`public/`): the region index `names.json` and the database slots `<region-index-path>/v<version>/{db.json, files}`. The private part (`private/`) SHALL never be reachable, by construction of the served root — no rewrite or deny rules required.

#### Scenario: Public data reachable
- **WHEN** a client requests names.json or a db.json in a version slot
- **THEN** the request succeeds with the file content and an appropriate content type

#### Scenario: Private data unreachable
- **WHEN** a client requests any path under the private part (e.g. check records or staging)
- **THEN** the server responds 404 without exposing private content

### Requirement: Anonymous read-only access
The web server SHALL serve all data without authentication or client-side credentials, SHALL NOT offer any write endpoint, and SHALL mount the repository read-only so it cannot modify the served data.

#### Scenario: No login
- **WHEN** a client requests any public file
- **THEN** no login or credential exchange is required

#### Scenario: Server cannot modify repository
- **WHEN** the container runs
- **THEN** its view of the repository is read-only and the image contains no write path to it

### Requirement: Static content web server
The image SHALL contain a static file web server (nginx-based) configured with the served root and JSON content types for names.json and db.json; it SHALL run as a non-root worker and SHALL not require a dynamic backend.

#### Scenario: Correct content types
- **WHEN** a client requests names.json or a db.json
- **THEN** the response content type is `application/json`

#### Scenario: Non-root execution
- **WHEN** the container starts
- **THEN** the web server worker processes run without root privileges

### Requirement: Compose orchestration
A docker-compose file SHALL orchestrate the mapgen regeneration container (writer) and the web server container (reader) sharing one repository volume; the web server SHALL mount the repository read-only, the mapgen container SHALL mount it writable, and both SHALL run one pass / server lifetime per invocation without internal scheduling.

#### Scenario: Both containers started together
- **WHEN** the compose file is started
- **THEN** the mapgen pass runs against the shared repository and the web server serves its public part

#### Scenario: Server sees regenerated data
- **WHEN** the mapgen container places a new database in the shared repository
- **THEN** the web server serves the new slot without restart

### Requirement: Image contents
The web server image SHALL identify its base web server software and version, and SHALL provide the served-root configuration and the JSON content-type mapping as part of the image.

#### Scenario: Contents inspectable
- **WHEN** an operator inspects the image
- **THEN** the bundled web server software version and configuration are determinable
