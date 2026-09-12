## Purpose

Hierarchical region index that maps every import id to localized display names and defines the tree structure used both by the client UI and as the layout hint for the server directory structure.

## ADDED Requirements

### Requirement: Hierarchical structure
The region index SHALL define a tree of regions. Internal nodes SHALL have children; leaf nodes SHALL reference import ids from the imports manifest. The tree SHALL be usable both for client display and as the layout hint for placing databases on the server.

#### Scenario: Tree with internal nodes and leaves
- **WHEN** the region index contains top-level regions with nested children
- **THEN** the deepest nodes reference import ids and the intermediate nodes group them into a hierarchy

### Requirement: Localized names
Every node in the region index SHALL carry display names keyed by language. Clients SHALL render the node name in the user's preferred language, falling back to a default when the preferred language is absent.

#### Scenario: Name in preferred language
- **WHEN** a client displays a node and the node has a name in the client's preferred language
- **THEN** the client shows that name

#### Scenario: Preferred language absent
- **WHEN** a client displays a node without a name in the preferred language
- **THEN** the client falls back to a default name for that node

### Requirement: Leaf id consistency
Every leaf node in the region index SHALL reference an import id that exists in the imports manifest, and every import id in the manifest SHALL be referenced by exactly one leaf node.

#### Scenario: Leaf without matching import
- **WHEN** the region index contains a leaf id that is not present in the imports manifest
- **THEN** the region index is invalid and the regeneration process refuses to run

#### Scenario: Import without matching leaf
- **WHEN** the imports manifest contains an id that no region index leaf references
- **THEN** the region index is invalid and the regeneration process refuses to run

### Requirement: Server layout hint
The regeneration process SHALL use the region index tree to determine where the database of each import is placed in the server directory structure.

#### Scenario: Placement follows the tree
- **WHEN** an import's database is placed on the server
- **THEN** it lands at the position implied by the leaf's path in the region index tree

### Requirement: Client display
A client SHALL be able to render the complete region index tree with localized names without any additional data.

#### Scenario: Full tree rendered from index alone
- **WHEN** a client has only the region index
- **THEN** it can render the full hierarchy and names for any supported language
