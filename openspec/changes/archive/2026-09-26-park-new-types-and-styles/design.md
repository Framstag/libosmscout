# Design

## Context

Motivation is in proposal.md — Why. This section only lists the measurements and
constraints that shape the approach, all taken on branch
`further-types-styles-symbols-and-fixes` at `5452ef13a` against `origin/master`
at `cf9313e8d` (merge base `1cbb8e19b`).

```
branch:  11 commits ahead of the merge base, 213 commits behind master
map.ost  3640 -> 8281 lines
types    master 665, branch 1519  ->  863 newly introduced names, 9 renamed away
parked   854 = 863 minus the 9 renamed worship types (kept active, see proposal)
styles   420 [TYPE ...] selector sites across 17 .oss files reference new types
```

Constraints that drive the decisions:

- `libosmscout-map/src/osmscoutmap/oss/Parser.cpp:1111` and `:1132` report an
  unknown type in a style selector as a warning and skip the rule; `:405` does the
  same for a `GROUP` declaration of the `ORDER WAYS` section. A forgotten active
  reference to a parked type therefore does not fail the load — it silently drops
  the rule and warns — which is why the reference checks below are tasks and not
  assumptions.
- Multi-type selectors exist and mix parked with active types, for example
  `stylesheets/include/aeroway.oss:96` and `stylesheets/cycle.oss:417`. A whole
  block can not be commented as a unit.
- `.oss` files are attached through `MODULE "include/<name>"` lines, for example
  in `stylesheets/standard.oss:308-320`. `include/office.oss` references only
  parked types, so it has a cheaper parking lever available.
- `.github/workflows/mapgen_image.yml:119` derives the module list from the
  `MODULE` lines in `map.ost` and requires those files beside it.
- Type blocks were reordered as well as added, and type ids are ordinal, so
  existing databases must be regenerated. `guidelines/FileFormatVersion.md:113`
  states that type configuration edits never bump `FILE_FORMAT_VERSION`, so the
  mismatch is not caught at open time.
- Only two non-style files reference newly introduced type names:
  `Tests/src/StyleConfigSymbolsTest.cpp` (as fixtures) and
  `libosmscout-client-qt/src/osmscoutclientqt/SearchModule.cpp` (as inert
  name-to-size strings).
- 18 new capability specs and 19 archived change records are landed unchanged,
  by explicit decision.

## Goals / Non-Goals

**Goals:**

- The shipped type configuration exposes only the released type set.
- The parked set stays in the repository, in place, locatable and countable by a
  single marker, so enabling it later is a localized edit.
- Every shipped stylesheet loads, and no active rule references a parked type.
- Improvements for types that already existed, including the worship renames,
  stay active.

**Non-Goals:**

- Measuring or improving lookup performance and index size for the new types;
  that analysis is the reason for parking, not part of this change.
- Building unparking tooling, a runtime switch, or a type-configuration module
  mechanism for parked types.
- Editing the landed type definition capability specs.
- Bumping `FILE_FORMAT_VERSION` or providing migration code for old databases.
- Removing the symbols, patterns or icons that belong to parked types.

## Flow

The pipeline is not a runtime flow, so the diagram below is the sequence of
build and verification steps this change performs.

```text
  parked-set computation
  ======================

  branch head 5452ef13a                 origin/master cf9313e8d
         |                                      |
         |<-------------- merge (D1) -----------|
         v
   merged working tree
         |
         |  types(branch) - types(master) - rename targets
         v
   parked set: 854 names
         |
         +--> map.ost, motorways.ost : wrap each parked TYPE block,
         |                            bracket with PARKED-NEW-TYPE (D2)
         |
         +--> 17 .oss files          : comment selector lines, or drop
         |                            parked names from mixed lists (D3)
         |
         +--> standard.oss           : comment MODULE "include/office"
         |                            (whole module is parked content)
         |
         +--> Tests                  : retarget GetPatternNames fixtures
         v

  verification
  ============

   parked set (854)                 merged type config
         |                                  |
         |  forward check: 0 active refs    |
         +--------------------------------->|
         |                                  |
         |  reverse check: every active     |
         |  [TYPE x] resolves           <---+
         |                                  |
         +--> load every shipped stylesheet -+
         |                                  |
         +--> cmake build + ctest            |
```

## Decisions

### D1 — Bring the branch up to date by merging master

Merge `origin/master` into the branch as one merge commit, then park on top.

- Alternatives: (a) rebase the 11 commits onto master — replays very large
  `.ost`/`.oss` rewrites 11 times and multiplies conflict resolution; (b)
  cherry-pick the wanted parts onto a fresh branch from master — discards the
  review history of PR #1782 and the archived change records that document the
  parked set.
- Rationale: the branch already carries a master merge (`5452ef1`), so merging is
  the established pattern here. Resolving conflicts once, against the final
  state, is far cheaper than doing it per replayed commit.
- Risk: master also moved in the same stylesheets (`standard.oss`,
  `include/roads.oss`, `include/route.oss` and others changed in the 213 commits),
  so conflicts are expected in exactly the files this change edits.

### D2 — Park by in-place line comments behind one marker

Each parked `TYPE` block in `stylesheets/map.ost` and
`stylesheets/motorways.ost` gets its lines commented with `//`, bracketed by a
`PARKED-NEW-TYPE` marker so the whole set is greppable and countable.

- Alternatives: (a) wrap blocks in `/* ... */` — nested comments already exist in
  these files and a mis-scanned block boundary silently comments out active
  types; (b) move parked definitions into a separate `.ost` module that no
  `MODULE` line references — cleaner end state, but it rewrites the merge diff
  instead of shrinking it and makes review of the PR nearly impossible; (c)
  delete the definitions and rely on git history for the later unpark — defeats
  the stated goal of enabling them with a small edit.
- Rationale: line comments cannot nest and cannot accidentally take active
  content with them, and every marked line is trivially reviewed in a diff.
- Risk: block boundary detection is the one mechanical step that can go wrong
  silently, so it is verified by counting marked definitions against the parked
  set, plus the reverse resolvability check over the whole stylesheet set.

### D3 — Park styles per type name, with a marker note wherever a parked name is removed from a mixed list

For each parked name: comment the rule when the rule's selector names only
parked types; when a selector list mixes parked and active types, drop the parked
names from the list and leave a marker line above the rule that names them. The
same treatment applies to the `GROUP` declarations of the `ORDER WAYS` section,
which also resolve type names (`Parser::WAYGROUP`, `libosmscout-map/src/osmscoutmap/oss/Parser.cpp:405`
— the only other construct besides `STYLEFILTER_TYPE` at `:1100` that does).
`SYMBOL <name>` blocks do not resolve types and stay untouched.

- Alternatives: (a) comment style *blocks* unconditionally — silently drops the
  styling of active types that share a selector list with parked ones, which
  directly contradicts the requirement that existing types stay fully active;
  (b) switch off a whole `.oss` module through its `MODULE` line, which was the
  original plan for `include/office.oss` — rejected once the content was known:
  `include/office.oss` keeps 30 active rules for types that are no longer
  defined, so the file would be loadable only because it is unreachable, and
  re-enabling the module before unparking the types would break the stylesheet
  set. Commenting the 30 rules keeps one uniform mechanism and leaves every
  shipped file loadable on its own; (c) leave the rules and remove the types
  from the type configuration only — the rules would keep loading as warnings
  and silently never apply, which is worse than either.
- Rationale: per-name surgery is the only granularity that preserves active
  styling, and the marker line records exactly where a parked name has to be put
  back. `include/office.oss` loses all 30 rules and keeps its colours and its now
  empty `GROUP` blocks, so it stays a valid, loadable module.
- Risk: an inline rule can share its line with the closing brace of an enclosing
  block (`[TYPE landuse_religious] AREA { ... }      }`); commenting the whole
  line would take that brace with it. Measured: one such line in
  `include/landuse.oss`. The transformer therefore ends the commented range at
  the rule's own closing brace and leaves the remainder of the line active, and
the brace balance of every stylesheet is checked against the pre-change tree.

### D4 — Retarget the pattern enumeration tests instead of deleting them

`Tests/src/StyleConfigSymbolsTest.cpp` gains three cases in this PR; two of them
use `landuse_apiary` and `landuse_forestry`, both parked. They are retargeted to
`landuse_cemetery` and `leisure_garden`.

- Alternatives: (a) delete the new cases — loses coverage of a new public API
  (`StyleConfig::GetPatternNames()`), which stays active; (b) keep
  `landuse_apiary` active as an exception — leaves one unreleased type in the
  shipped configuration, contradicting the spec.
- Rationale: measured against the merged stylesheets, exactly three active types
  carry pattern rules (`landuse_cemetery`, `leisure_garden`, `natural_scrub`), so
  two of them keep the existing two-entry assertions intact.
- Risk: if the merge with master changes which active types carry pattern rules,
  the fixtures must be re-measured against the merged tree rather than reused.

### D5 — Land the openspec artifacts unchanged

The 18 new capability specs, the modified `symbol-scan-tool` spec and the 19
archived change records are landed as they are; only the change artifacts for
this work are added.

- Alternatives: (a) hold back the capability specs until the types are unparked —
  keeps `openspec/specs` truthful, but splits the PR and loses the per-category
  type lists in the same review; (b) add a parked note to each capability spec —
  honest, but 18 extra edits in a change that is about not editing them.
- Rationale: the archived records are the unpark reference, and the explicit
  decision was to keep this PR's openspec content whole.
- Risk: several capability specs describe types that are inactive after this
  change, so a later reader may treat the parked comments as a defect. Mitigated
  by the archived records, by this change's own `parked-type-definitions` spec
  and by the marker itself.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| A parked type keeps an active style reference → its rule or `GROUP` entry is dropped with only a warning, so the loss is silent | Forward check over every shipped stylesheet, and a comparison of the loader's warning names against the pre-change tree; both are tasks |
| An active type loses its styling because it shared a selector list with a parked type | Per-name surgery (D3), plus a check that every type active before the change still has at least one active rule |
| Block boundary detection comments active types out | Count marked definitions against the parked set; marker must not appear on any released definition |
| Merge conflicts in the very files this change edits, resolved once and wrongly | Merge (D1) is a separate commit; conflict resolution is reviewed before any parking happens |
| Parked styles remain in `include/office.oss` and are only switched off by a module line | Called out in the commit message and in the tasks |
| Landed capability specs describe inactive types (D5) | Noted in proposal Impact; the marker and the archived records point at the parked set |
| Old databases open successfully but misparse, because the version does not change | Stated as BREAKING in the proposal; re-import is a required step, and the data regeneration path is external to this repository |
| Master moves again while this work is in progress | Re-merge before review; the parked set is derived from type *names*, so it survives re-application |

## Migration Plan

1. Merge master, then apply the parking commits, so the park itself is one
   reviewable, revertable unit.
2. Regenerate all map data from the merged type configuration before any release
   that uses it; existing databases are not interchangeable (see proposal —
   BREAKING).
3. Rollback is `git revert` of the park commits: the parked definitions are
   present, so reverting restores the active type set without touching history.

## Open Questions

- Which measurement gates the later unpark, and for which platforms, is not
  decided here; it does not change the specs, the approach or the tasks.
- Whether the parked set eventually becomes its own type configuration module
  rather than in-place comments can be decided when it is unparked.
