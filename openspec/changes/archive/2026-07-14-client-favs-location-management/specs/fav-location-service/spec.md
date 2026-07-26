## ADDED Requirements

### Requirement: Service creates default fav locations file on first access
The system SHALL create an empty JSON file at the configured path when `FavoriteLocationService` is first constructed, if the file does not already exist. The file SHALL contain an empty JSON object `{}`.

#### Scenario: First access creates file
- **WHEN** `FavoriteLocationService` is constructed with a path that does not exist
- **THEN** the file SHALL be created containing `{}`

#### Scenario: Existing file is not overwritten
- **WHEN** `FavoriteLocationService` is constructed with a path to an existing file
- **THEN** the file content SHALL NOT be modified

### Requirement: Service loads and saves groups with favs
The system SHALL persist a one-level grouping structure. Each group has a name (string). Each group contains zero or more favs. Each fav has a name (string) and a geographic coordinate (latitude, longitude). The data model SHALL use extensible attribute maps (`std::map<std::string, std::string>`) on both groups and favs to support future attributes without schema changes.

#### Scenario: Save and reload preserves all data
- **WHEN** a group with multiple favs is saved to file
- **THEN** reloading from the same file SHALL return identical groups and favs

#### Scenario: Empty group is persisted
- **WHEN** a group with no favs is saved
- **THEN** reloading SHALL return that group with an empty fav list

#### Scenario: Extensible attributes on groups
- **WHEN** a group has custom attributes set in its attribute map
- **THEN** those attributes SHALL be preserved across save/reload

#### Scenario: Extensible attributes on favs
- **WHEN** a fav has custom attributes set in its attribute map
- **THEN** those attributes SHALL be preserved across save/reload

### Requirement: Service supports CRUD for groups
The system SHALL provide methods to add, delete, and list groups. Adding a group with a name that already exists SHALL return an error or false. Deleting a non-existent group SHALL return an error or false.

#### Scenario: Add group succeeds
- **WHEN** a new group name is added
- **THEN** the group SHALL appear in the group list

#### Scenario: Add duplicate group fails
- **WHEN** a group name that already exists is added
- **THEN** the operation SHALL fail and the existing group SHALL remain unchanged

#### Scenario: Delete group succeeds
- **WHEN** an existing group is deleted
- **THEN** the group SHALL be removed from the group list

#### Scenario: Delete non-existent group fails
- **WHEN** a group name that does not exist is deleted
- **THEN** the operation SHALL fail

### Requirement: Service supports CRUD for favs within a group
The system SHALL provide methods to add, delete, and rename favs within a group. Adding a fav with a duplicate name within the same group SHALL fail. Deleting a non-existent fav SHALL fail. Renaming a fav to an existing name SHALL fail.

#### Scenario: Add fav to group succeeds
- **WHEN** a fav with a unique name is added to an existing group
- **THEN** the fav SHALL appear in that group's fav list

#### Scenario: Add fav with duplicate name fails
- **WHEN** a fav with a name that already exists in the group is added
- **THEN** the operation SHALL fail

#### Scenario: Add fav to non-existent group fails
- **WHEN** a fav is added to a group that does not exist
- **THEN** the operation SHALL fail

#### Scenario: Delete fav from group succeeds
- **WHEN** an existing fav is deleted from its group
- **THEN** the fav SHALL be removed from that group's fav list

#### Scenario: Rename fav succeeds
- **WHEN** an existing fav is renamed to a new unique name
- **THEN** the fav SHALL have the new name

#### Scenario: Rename fav to duplicate name fails
- **WHEN** an existing fav is renamed to a name already used in the same group
- **THEN** the operation SHALL fail

### Requirement: Service provides lookup by group and by name
The system SHALL provide methods to retrieve all favs in a group, and to find a specific fav by name within a group.

#### Scenario: Lookup favs by group
- **WHEN** a group name is queried
- **THEN** all favs in that group SHALL be returned

#### Scenario: Lookup non-existent group returns empty
- **WHEN** a non-existent group name is queried
- **THEN** an empty list SHALL be returned

#### Scenario: Lookup fav by name
- **WHEN** a group name and fav name are queried
- **THEN** the matching fav SHALL be returned

#### Scenario: Lookup non-existent fav returns null
- **WHEN** a non-existent fav name is queried within an existing group
- **THEN** null/empty SHALL be returned

### Requirement: JSON file format is human-readable
The persisted JSON file SHALL use indented formatting (2-space indent) for readability. The file SHALL use UTF-8 encoding.

#### Scenario: File is indented
- **WHEN** the file is written
- **THEN** it SHALL use 2-space indentation

#### Scenario: File is UTF-8
- **WHEN** the file is written
- **THEN** it SHALL be UTF-8 encoded

### Requirement: Service is thread-safe for read operations
Read operations (get groups, get favs, lookup by name) SHALL be safe to call from multiple threads concurrently. Write operations (add, delete, rename, save) SHALL be serialized.

#### Scenario: Concurrent reads do not block each other
- **WHEN** multiple threads call read methods simultaneously
- **THEN** all threads SHALL complete without data races

#### Scenario: Write blocks concurrent reads
- **WHEN** a write operation is in progress
- **THEN** concurrent read operations SHALL wait for the write to complete
