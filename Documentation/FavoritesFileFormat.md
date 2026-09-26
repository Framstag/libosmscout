# Favorites file format

The client library (`libosmscout-client`, `FavoriteLocationService`) persists favorite locations in a
single JSON file. This document describes that file: its structure, the versions it can carry, and the
compatibility rules a reader and a writer follow.

The file is client data, not a database or type-config format, so the `FileFormatVersion.md` rules for
database format bumps do not apply to it. The version described here is the file's own.

## Where the file lives

The application chooses the path; the library only reads and writes it. JavaScout uses
`<configDir>/favorites.json`, next to `config.properties`. `FavoriteStore` owns the service instance and
serialises every access to it.

## Top level

```json
{
  "formatVersion": 1,
  "groups": [ ... ]
}
```

- `formatVersion` — integer, the version of the format the file was written in. Written on every save.
- `groups` — array of group objects, **in the user-defined group order**. The sequence of this array *is*
  the group order: it is what a reader reports, what a positional group move changes, and what a save
  persists. There is no separate order field.

A file with no `groups` key (for example `{}`) holds no groups and is readable.

## Group object

```json
{
  "name": "Work",
  "attributes": { "color": "FF5733" },
  "favorites": [ ... ]
}
```

- `name` — string, unique among the groups. A group is looked up by this name; renaming a group keeps its
  position in the array.
- `attributes` — object of string key/value pairs. Extensible: unknown keys are preserved across a
  read/write cycle and are not interpreted by the library.
- `favorites` — array of favorite objects, **in the order the group shows them**.

## Favorite object

```json
{
  "name": "Office",
  "lat": 51.5,
  "lon": 7.25,
  "attributes": { "starred": "true", "starredPosition": "100" }
}
```

- `name` — string, unique among the favorites of its group. When a favorite is moved into another group
  that already holds a favorite of that name, the move is refused and nothing changes.
- `lat`, `lon` — numbers, the geographic coordinate.
- `attributes` — object of string key/value pairs, extensible like the group attributes.

## Attributes the library interprets

The attribute maps are the extension point of the format, so a reader preserves unknown keys. Three keys
are interpreted by the library today:

| Key | On | Value | Meaning |
|-----|----|-------|---------|
| `color` | group | 6 hex characters, e.g. `FF5733` | Color of the group. Absent means no color. An empty string clears it. |
| `starred` | favorite | `"true"` | The favorite is starred. Absent (or any other value) means not starred. |
| `starredPosition` | favorite | decimal integer as a string, e.g. `"100"` | Place of a starred favorite in the starred order. |

`starredPosition` is **owned by the library** and is not part of the contract a caller relies on. Callers
arrange starred favorites with positional operations (`MoveStarred` and the same operation in the store,
the Java bindings and the dialog); they never set or maintain this value. The library spaces the values
apart so that later moves stay cheap, writes the whole order again when a move needs it, and tolerates
hand-edited files: a missing, unknown or non-numeric value sorts after the known ones and ties fall back to
group order and favorite name, so a reader always produces a total order. When a file that is saved would
otherwise be inconsistent (a starred favorite without a value, an unparsable value, or two favorites
claiming the same place), the save writes the reported order back with the library's own spaced values, so
a saved file is always self-consistent.

## The three orders in the file

| Order | Where it lives |
|-------|----------------|
| Group order | the sequence of the `groups` array |
| Favorite order inside a group | the sequence of that group's `favorites` array |
| Starred order (spans groups) | the `starredPosition` values of the starred favorites |

None of the three is derived from the others, and none is derived from a name.

## Example of a current file

Written by the library, indented with two spaces. Object keys come out in alphabetical order because the
JSON object type the library uses is sorted by key; the order that matters is the order of the arrays.

```json
{
  "formatVersion": 1,
  "groups": [
    {
      "attributes": {
        "color": "FF5733"
      },
      "favorites": [
        {
          "attributes": {
            "starred": "true",
            "starredPosition": "100"
          },
          "lat": 51.5,
          "lon": 7.25,
          "name": "Office"
        }
      ],
      "name": "Work"
    },
    {
      "attributes": {},
      "favorites": [
        {
          "attributes": {},
          "lat": 51.4,
          "lon": 7.2,
          "name": "Home"
        }
      ],
      "name": "Home"
    }
  ]
}
```

## The pre-version form (version 0)

Files written before the file carried a version use the same group and favorite objects, but key the groups
by group name, which cannot express an order:

```json
{
  "groups": {
    "Work": {
      "name": "Work",
      "attributes": { "color": "FF5733" },
      "favorites": [ { "name": "Office", "lat": 51.5, "lon": 7.25, "attributes": { "starred": "true" } } ]
    },
    "Home": {
      "name": "Home",
      "attributes": {},
      "favorites": []
    }
  }
}
```

A file without a `formatVersion` key is this form. It is read: no group, favorite or attribute is lost, and
because the form carries no order, the groups are reported **sorted by group name**. The next save writes
the current form, so the file converges to version 1 on first write and carries the order from then on.

## Compatibility rules

| Version in the file | Read | Written by this client |
|---------------------|------|------------------------|
| absent (pre-version form, reported as version 0) | read; groups sorted by name | rewritten as version 1 on the next save |
| 1 (current) | read in the stored order | yes |
| 2 or higher (newer than the client knows) | **not** read as groups; the reported version and the unsupported state are exposed instead | **never** — every write over that path fails and the file is left unchanged |

A file whose content cannot be parsed is treated like an unsupported one: nothing is reported and the file
is not written over, so a file the client does not understand is never destroyed.

The unsupported state is readable so that a client can tell "written by a newer version" apart from "no
favorites". A reader that ignores it shows an empty favorites list where the user still has favorites, and
the first save would destroy them; that is exactly what the rules above prevent.

| Layer | How the state is read |
|-------|-----------------------|
| `FavoriteLocationService` | `GetFileFormatVersion()` (or `UnknownFileFormatVersion`), `IsFileFormatSupported()` |
| `FavoriteStore` | `GetFileFormatVersion()`, `IsFileFormatSupported()` |
| Java (`OSMScoutClient`) | `getFavoriteFileFormatVersion()`, `isFavoriteFileFormatSupported()` |
| JavaScout dialog | reports that the file was written by a newer version, refuses to save, leaves the file alone |

## Writing

A save writes to `<path>.tmp` first and then renames it over the file, so a reader never observes a partial
file. The document is indented with two spaces and is UTF-8 encoded. The written document always carries
`formatVersion` set to the version the client writes.

## Removing pre-version support

Reading the pre-version form exists so that upgrading does not lose favorites. It is a compatibility path
with a limited life: once enough releases have shipped for users' files to have been rewritten in the
versioned form, the version 0 read path can be removed, and a version 0 file can then be treated like any
other unsupported version. That removal is recorded as a follow-up entry in `TODO.md`; the version field
described here is what makes the removal decidable later without guessing.
