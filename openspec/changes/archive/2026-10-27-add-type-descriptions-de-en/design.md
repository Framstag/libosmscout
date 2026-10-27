# Design

## Context

See proposal.md — Why for the motivation. This section lists only the measurements and constraints
that shape the approach.

```
type definitions in the shipped configuration
  stylesheets/map.ost              active 640   PARKED-NEW-TYPE 849   FEATURES block 6
  stylesheets/motorways.ost        active  23   parked 5 (already carry // DESC)
  stylesheets/basemap.ost          active  11   parked 2
  stylesheets/contour_lines.ost    active   3
  described today                  types 10, features 6
```

Constraints that drive the decisions:

- `guidelines/FileFormatVersion.md` classifies "groups / descriptions" as a **content-only** edit:
  `types.dat` is self-describing and count-prefixed, so a description never needs a
  `FILE_FORMAT_VERSION` bump (current value 27). The same guideline states that `.ost` edits still
  make previously imported data a regenerable artifact — an old database simply carries the old
  descriptions.
- `TypeConfig::StoreToDataFile()` writes the descriptions of every non-internal type
  (`libosmscout/src/osmscout/TypeConfig.cpp:1625`) and of every feature that has any (`:1579`);
  `LoadFromDataFile()` reads both back (`:1459`, `:1509`). The write and the read already agree.
- `TypeInfo::GetDescription(languageCode)` returns an empty string when the language is absent
  (`TypeConfig.cpp:277`), so "no description" is already expressible without a new type.
- The SWIG Java binding already wraps both lookup forms — `%template(StringStringUnorderedMap)
  std::unordered_map<std::string,std::string>` in
  `libosmscout-binding/src/swig/osmscout/OSMScoutTypes.i:14` plus the plain include of
  `TypeConfig.h` in `TypeConfig.i`.
- JavaScout does **not** use the SWIG binding. It uses `libosmscout-client-java`, where
  `PoiEntry.objectType` and `LocationEntry.objectType` are filled with `GetType()->GetName()`
  (`libosmscout-client-java/src/OSMScoutClient.cpp:8633`, `:181`, `:193`, `:205`, `:3640`) and the
  UI prints that string verbatim (`PoiSearchOverlay.java:225`, `SearchOverlay.java:212`,
  `CandidatePickerOverlay.java:191`).
- `OSMScoutClient.cpp` is one 8,000+ line translation unit; every search entry point already carries
  a long parameter list.
- The Kotlin binding's reader parses descriptions and throws them away in three places
  (`libosmscout-kotlin/.../TypeConfig.kt:38`, `:51`, `:67`). It is a demo implementation that no
  shipped application uses, and it declares `FILE_FORMAT_VERSION = 19u` while the library writes 27,
  so it currently rejects every current database — a pre-existing defect, recorded in `TODO.md` and
  not fixed here.
- The library-defined features (`Bridge`, `Tunnel`, `Layer`, `MaxSpeed`, `Operator`, `Brand`,
  `Elevation`, ...) exist without a `FEATURE` line in the stylesheets; the `FEATURES` block only
  attaches descriptions to the ones named there.

## Goals / Non-Goals

**Goals:**

- Descriptions live where the type configuration already carries them, so every client that reads a
  type configuration gets them without a new data source.
- A caller can tell "no description" from "description", and the fallback order is decided in one
  place rather than in every client.
- A new type without a description fails a test instead of reaching an application as a machine name.
- An application built before this change keeps working against a database generated after it, and
  vice versa.

**Non-Goals:**

- A translation workflow, an external translation format or a runtime language file.
- Descriptions for languages beyond German and English.
- Localizing anything else (`ObjectDescription` section headers, POI category names, units).
- Adding descriptions to the runtime-registered synthetic types (`_route`, `_favorite`, `_track`,
  ...) — they are not part of a type configuration read from a database.
- Implementing description storage in the Kotlin binding: it is a demo implementation that no shipped
  application uses (see D5).
- Changing the map repository pipeline or triggering a regeneration of published databases.
- Fixing the Kotlin binding's stale `FILE_FORMAT_VERSION`.

## Flow

```text
JavaScout                JNI bridge (ClientData)          TypeConfig             UI
   |                             |                             |                  |
   | build() with language "de"  |                             |                  |
   |---------------------------->| store in ClientData         |                  |
   | searchPOIs(cat, lat, lon, r)|                             |                  |
   |---------------------------->|                             |                  |
   |                             | obj->GetType()->GetName()   |                  |
   |                             | GetTypeInfo(name)           |                  |
   |                             |---------------------------->|                  |
   |                             | GetDescription("de")        |                  |
   |                             |---------------------------->|                  |
   |                             |  "" --> GetDescription("en")|                  |
   |                             |---------------------------->|                  |
   |                             |  "Bank" | ""                |                  |
   |  PoiEntry(objectType, objectTypeDescription)              |                  |
   |<----------------------------|                             |                  |
   |                                                                              |
   | description empty ? show objectType : show description                       |
   |----------------------------------------------------------------------------->|

   An older database carries no descriptions: every GetDescription() answers "" and
   the UI takes the objectType branch - the behaviour before this change.
```

## Decisions

### D1 — Descriptions live in the `.ost` `DESC` blocks

Each described type gets `DESC` with `en:` and `de:` inside its own definition, exactly as the ten
descriptions that exist today do (`stylesheets/map.ost:188`, `stylesheets/motorways.ost:47`).

- Alternatives: (a) a per-language resource file shipped beside the application or the database — a
  second source of truth that drifts from the type configuration, and it reaches only the client that
  knows about it; (b) a Java/JSON translation table inside each app — the same drift, repeated per
  app, and invisible to C++/Qt/NaviVeylin consumers; (c) deriving the wording mechanically from the
  type name or its tag conditions — yields precisely the machine wording this change removes.
- Rationale: `DESC` already flows through `types.dat` to every client, needs no new format, no new
  file and no new API, and stays attached to the definition it describes.
- Risk: the description only reaches a client after the database is regenerated, so the effect of
  this change is delayed to the next import (see Migration Plan).

### D2 — The resolution order is: requested language, then English, then nothing

A resolution that cannot answer in the requested language answers with the English description when the
type has one, and otherwise reports that no description is present. The caller falls back to the
technical type name. The order is implemented once, in the library, and used by the bridge and by the
bindings, so no client repeats it; the exact-language lookup keeps its present behaviour of returning
an empty string.

- Alternatives: (a) return the technical type name when nothing is found — the caller can no longer
  distinguish an undescribed type from one whose description equals its name, which the
  `type-description-lookup` spec forbids; (b) return the first description of any language — a German
  application would silently show a French string once a third language exists; (c) leave the whole
  order to each caller, above the library — every client reimplements it and the apps diverge, which
  is the state this change removes.
- Rationale: one place decides for every language consumer — the JNI bridge, the SWIG binding and the
  C++ clients — and the empty answer remains a fact about the configuration rather than a display
  decision.
- Risk: the technical name is then produced in the UI rather than in the bridge, so each UI site
  needs the fallback — covered by the modified UI specs and their scenarios.

### D3 — The language is a client setting, not a parameter of every call

The builder gains a description language, and a running client can change it. Results are resolved
against the current setting.

- Alternatives: (a) a language parameter on every search call — changes the signature of every
  search method and every JNI entry point in a file where the call sites are already parameter-heavy;
  (b) hardcoding German — wrong for the English-speaking user and for every non-German deployment;
  (c) reading the language from the database metadata `db.json` — it describes the languages of the
  *names stored in the data*, not the language the user reads.
- Rationale: the language belongs to the session, not to a query; JavaScout sets it from the
  platform locale with an English fallback and can offer a switch, and no existing call changes its
  shape.
- Risk: a client that never sets the language gets the default; the default is documented as English,
  which answers with a description for most types and never with a wrong-language string.

### D4 — The bridge resolves the description while serializing a result

`PoiEntry`, `LocationEntry` and `ObjectDescription` carry the resolved description as a plain string
next to the technical name.

- Alternatives: (a) a separate native lookup the UI calls per row (`getTypeDescription(typeName,
  language)`) — one JNI crossing per visible row while a result list scrolls, and the UI needs a
  cache to stay smooth; (b) handing the whole type-to-description map to Java once — a large
  structure to copy, and the fallback would move into Java (see D2); (c) resolving in the UI from a
  Java-side cache keyed by type name — the same per-row cost as (a) plus a cache to invalidate on a
  language change.
- Rationale: the bridge holds the loaded `TypeConfig` and the object already, so the description is
  one string on a result that is being built anyway; a language change affects the next search, which
  is when the UI reloads its list regardless.
- Risk: the fields are additive, so an application and a native library of different vintages keep
  working; a stale native library leaves `objectTypeDescription` null and the UI must treat null as
  "no description".

### D5 — Demo bindings are out of implementation scope

The requirement that a binding exposes the descriptions it reads stays general, and the bindings that
an application uses — the SWIG Java binding and the client bridge — are the ones this change
implements and verifies. The Kotlin binding is a demo implementation that no shipped application
uses, and it keeps its present behaviour.

- Alternatives: (a) implement the description storage in `libosmscout-kotlin` — work on code nothing
  runs, which could not even be verified against a real database because that reader rejects every
  current one for an unrelated reason (its `FILE_FORMAT_VERSION` is 19 against the library's 27);
  (b) delete the Kotlin binding or repair its reader — neither is this change's call, and both are
  larger than the gap they close; (c) narrow the requirement to the bindings an application uses —
  leaves the demo reader discarding values it already parses, and the contract stops describing what
  a future reader owes its callers.
- Rationale: the requirement describes the contract for any binding that reads a type configuration,
  while the change spends its work where a user can see the result.
- Risk: a demo binding keeps reporting every type as undescribed although the data carries the
  descriptions; recorded as a finding, and the requirement stays the specification a later change
  would satisfy.

### D6 — Wording rule, and an explicit list of types left without a description

A description is the noun phrase a user of a map application recognizes for the object, in the
language it is written in. It never restates the type identifier, never spells out tagging
vocabulary, and is never a word-for-word rendering of the other language.

A type is left without a description when its user-facing name cannot be established — an unclear or
overlapping tag meaning, a classification helper a user never encounters, or a pair of types whose
difference is not visible. Those names go into one reviewed list in the change, and the list is the
input for the coverage test (D8).

- Alternatives: (a) describe every type so nothing stays empty — forces guesses, and a wrong name is
  worse than none; (b) describe only the types an app shows today — the remaining definitions stay
  undescribed for the next application, which reopens the gap; (c) machine-translate the English to
  German — literal wording, which the `type-description-data` spec forbids.
- Rationale: the requirement is a name a user understands; where that is unknown, silence is honest
  and the list makes the decision reviewable.
- Risk: the list becomes a place to avoid work — mitigated by naming the reason for every entry and
  by the coverage test reporting a new type that is neither described nor listed.

### D7 — Parked types carry their descriptions inside the commented block

A parked definition gets its `//   DESC` lines between the `// TYPE` lines and the marker's block
end, following the five parked definitions in `stylesheets/motorways.ost` that already do this.

- Alternatives: (a) a separate list of descriptions keyed by parked type name — a second place that
  must be merged when the type is enabled, which is exactly the "localized edit" property the parked
  set exists for; (b) leaving parked types undescribed — the `type-description-data` spec requires an
  inactive definition to carry the same descriptions as an active one; (c) uncommenting the parked
  types to describe them — re-enables the parked set, contradicting its own change record.
- Rationale: enabling a parked type stays a single edit, and a reviewer of the parked block sees what
  the type will be called.
- Risk: commenting the `DESC` lines by hand can break the `//` prefix alignment or the block end;
  the coverage test reads parked definitions too, so a malformed block is caught.

### D8 — Coverage is enforced by a test over the shipped files

A test enumerates the shipped `.ost` files, extracts every type name — active and parked — and every
feature name, and asserts that each either carries both shipped languages or appears in the list of
deliberately undescribed names from D6.

- Alternatives: (a) no test — a type added later silently reaches applications as a machine name,
  which is the gap this change closes; (b) a test that requires a description for every type — turns
  the deliberate omissions into failures and pushes authors to guess; (c) asserting only on a
  hand-maintained list of described names — the list rots against the stylesheets.
- Rationale: the test fails on a new undescribed type and passes on a reviewed omission, so the
  decision to omit becomes explicit.
- Risk: a test that parses `.ost` source rather than a loaded configuration is a source-level check;
  it is chosen because a loaded configuration cannot see parked definitions at all, and the existing
  `Tests/src/TypeResolutionTest.cpp` already reads `PARKED-NEW-TYPE` markers out of the same file.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| A wrong user-facing name reaches the app, which is worse than a machine name | D6: leave it out when the name cannot be established; the omission list is reviewed; the OSM wiki is the source for tag meaning, not for wording |
| The effect is invisible until databases are regenerated | Stated in the proposal; the migration step is the map repository regeneration, and the fallback keeps old databases working |
| German wording is written as a translation of the English one | D6 forbids it; the coverage test asserts presence, not wording, so the wording is a review concern of the description commits |
| The omission list grows and hides work | Every entry names a reason, and a new type must be described or listed |
| A stale native library returns null descriptions while the Java side expects a string | D4: the field is additive and null is treated as "no description"; the app degrades to the old display |
| The parked-block edit breaks the marker block | D7: the coverage test reads parked definitions; the marker count is checked by the existing parked-type test |
| A test that parses `.ost` source breaks on formatting changes | The existing `TypeResolutionTest` parses the same file with the same assumptions; the extraction is shared with it where possible |
| Description edits change `types.dat`, so a published database and a fresh import differ without a version change | `guidelines/FileFormatVersion.md` classifies descriptions as content-only; the repository replaces the same version slot on regeneration, as it already does for content changes |

## Migration Plan

1. Land the `.ost` descriptions and the client changes in one change; no `FILE_FORMAT_VERSION` bump.
2. Regenerate the map repository databases from the updated type configuration on the next scheduled
   regeneration. Until then, clients show technical names as before; nothing breaks.
3. Rollback is `git revert`: the descriptions are inert data, the additive Java fields are ignored by
   a library that does not set them, and no database becomes unreadable.

## Open Questions

- Whether the German wording is reviewed by a native speaker before release, or only in review, is a
  process question that does not change the specs, the approach or the tasks.
- Whether the omission list eventually moves into the stylesheets (for example a marker beside the
  definition) can be decided when the list is first reviewed.
