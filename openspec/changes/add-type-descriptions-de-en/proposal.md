# Proposal

## Why

The type configuration has carried localized descriptions since the feature was introduced, and the
database metadata written at import time already stores them, but only a fraction of the shipped
configuration fills them in: of the 677 active type definitions across the shipped `.ost` files, ten
carry a description, and six features do. A client that wants to show a user what an object *is* has
nothing to show, so applications fall back to the technical type name - JavaScout prints
`amenity_bank` in its POI and location result lists, and `place_town` in its result cells. Nothing
reports that the localized information exists but is missing, so the gap is invisible until an app
is asked why it shows machine names.

The parked type set is in the same position: 849 definitions in `map.ost` plus a handful elsewhere
are retained, inactive, so that enabling them later is a localized edit. A type that is enabled
without a description arrives in an app with a technical name, which is exactly the state this
change removes for the released set.

## What Changes

- Every user-facing type definition in the shipped type configuration carries a German and an
  English description, whether it is active or parked.
- The user-facing features carry a German and an English description as well.
- Descriptions are written for the people who use an application, not for map data editors: they name
  what a user sees or looks for, and they avoid vocabulary that only belongs to OSM tagging.
- A German description is not a literal translation of the English one; each is the wording that
  fits the language and the region it is read in.
- A type whose correct user-facing name cannot be established is left without a description rather
  than given a guess, and stays without one until someone can establish the name.
- A client can ask the type configuration, in a language of its choosing, what a type is called, and
  receives a defined answer when the configuration has no description for that language.
- JavaScout shows the description of a type instead of its technical name in the POI and location
  result lists, in the result cells, and in the object description dialog.
- The Java binding and the client bridge hand the descriptions of the loaded type configuration to
  their callers instead of discarding them.
- Existing databases keep working without a description for every new type: until a database is
  regenerated from the updated type configuration, an application shows what it showed before.

## Capabilities

### New Capabilities

- `type-description-data`: the contract for the description content of the shipped type
  configuration - which types and features carry descriptions, in which languages, how the wording
  is chosen for the people who use an application, and that an uncertain name is left out.
- `type-description-lookup`: the contract for retrieving a type's description - the language a caller
  asks for, what a caller receives when the configuration has no description in that language, and
  that the shipped bindings expose the descriptions they load.

### Modified Capabilities

- `location-search-api`: a location search result exposes the description of its object type, not
  only the technical type name.
- `location-search-ui`: the location result list shows the description of a result's object type.
- `poi-search-api`: a POI search result exposes the description of its object type.
- `poi-search-ui`: the POI result list and the detail view show the description of a result's object
  type instead of its technical name.
- `long-press-candidate-picker`: the candidate list names each candidate's object type by its
  description rather than by its technical type name.

## Impact

Data - the shipped type configuration:

- `stylesheets/map.ost` - the `FEATURES` block and the active and `PARKED-NEW-TYPE` type
  definitions.
- `stylesheets/motorways.ost`, `stylesheets/basemap.ost`, `stylesheets/contour_lines.ost` - the
  remaining type definitions.
- `<database>/types.dat` - written at import time; carries the descriptions. No file format version
  change.

Client access:

- `libosmscout/include/osmscout/TypeConfig.h`, `libosmscout/src/osmscout/TypeConfig.cpp` - the
  per-type and per-feature description storage and lookup.
- `libosmscout-client-java/src/OSMScoutClient.cpp` - the JNI bridge, which resolves a type name
  today and does not ask for a description.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/` - `PoiEntry.java`,
  `LocationEntry.java`, `ObjectDescription.java`, `OSMScoutClient.java`,
  `OSMScoutClientBuilder.java`.
- `libosmscout-binding/src/swig/osmscout/TypeConfig.i` and `OSMScoutTypes.i` - the SWIG binding
  already wraps both lookup forms.

Applications:

- `JavaScout/src/main/java/com/framstag/libosmscout/PoiSearchOverlay.java`,
  `SearchOverlay.java`, `SearchResultCell.java`, `CandidatePickerOverlay.java`,
  `MainController.java`.

Tests and documentation:

- `Tests/src/` - a case that reads the shipped type configuration and reports which types lack a
  description, and cases for the lookup fallback.
- `JavaScout/src/test/java/com/framstag/libosmscout/` - a case for the display fallback.
- `guidelines/` or `Documentation/StyleSheetSyntax.txt` - the description syntax and the naming rule.

Outside this repository:

- `Framstag/NaviVeylin` consumes the C++ API and can use the descriptions once they exist; no change
  is made here.
