# Proposal

## Why

PR #1782 adds roughly 850 new type definitions with styles and symbols. Review
raised that every type gets its own lookup tiles in the index, so activating all
of them at once affects lookup performance and database size, and that this
analysis is still open — especially on mobile devices.

The parts of the PR that carry independent value should not stay blocked behind
that open question: the symbol rendering fixes, the improved symbols and patterns
for types that already exist, and the style corrections for those types.

## What Changes

- Type definitions that the PR introduces are no longer active in the shipped
  type configuration. They are retained in the repository in an inactive,
  machine-findable form, so enabling them later is a small, reviewable edit
  instead of a restoration from history.
- Style rules and symbols that belong to the newly introduced types are
  inactive in the same way.
- Everything that concerns types that already existed keeps working unchanged:
  improvements, corrections and renames of existing types, their styles,
  patterns and symbols.
- The symbol/pattern inspection additions and the symbol rendering fixes stay
  active.
- The shipped stylesheets must keep loading as a whole. No active style rule may
  reference an inactive type.
- **BREAKING**: every map database must be re-imported. Type definitions are
  stored by ordinal id, and this work removes, renames and reorders entries, so
  data imported with an earlier type configuration is not interchangeable with
  the new one. The file format version stays unchanged, so such a mismatch is
  not detected at open time — it must be handled by regenerating the data.

## Capabilities

### New Capabilities

- `parked-type-definitions`: the shipped type configuration contains only the
  released type set; type definitions and style rules that are not released yet
  are retained in an inactive, greppable form; the shipped stylesheets always
  load and resolve every type they reference; types that existed before the change stay
  fully active.

### Modified Capabilities

- None. The type definition capability specs that the PR brings in are landed
  as they are; this change does not edit them.

## Impact

Type configuration and styles:

- `stylesheets/map.ost` — the large majority of the additions are type
  definitions for types that become inactive
- `stylesheets/motorways.ost` — a smaller set of the same
- `stylesheets/include/*.oss` — 17 files whose style rules are partly or wholly
  about the new types; `include/office.oss` and parts of `include/religious.oss`
  are the extreme cases
- `stylesheets/standard.oss`, `stylesheets/cycle.oss`,
  `stylesheets/winter-sports.oss`, `stylesheets/public-transport.oss` — module
  selection and cross-cutting rules that reference the affected types

Code and tests:

- `Tests/src/StyleConfigSymbolsTest.cpp` — the new pattern enumeration cases are
  written against a type that becomes inactive and have to use an active type
  instead
- `libosmscout-map/include/osmscoutmap/StyleConfig.h`,
  `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` — the new pattern
  enumeration API stays active and unchanged
- `Demos/src/SymbolsAll.cpp` — stays active and unchanged
- `libosmscout-client-qt/src/osmscoutclientqt/SearchModule.cpp` — the new size
  hints for place types stay active; they are name-keyed hints and activate no
  type by themselves

Data and repository:

- All map databases must be regenerated (see the breaking note above);
  `guidelines/FileFormatVersion.md` explains why the version constant is not
  touched for type configuration edits
- `openspec/specs/*` — 18 new type definition capability specs and one modified
  capability spec are landed unchanged, so several of them describe types that
  are inactive after this change
- `openspec/changes/archive/2026-08-*` — 19 archived change records are landed
  unchanged; they are the reference for what is parked and why
