# Type descriptions

Guideline for the localized descriptions that the type configuration carries per type and per
feature (`DESC` in `stylesheets/*.ost`).

## What a description is

A description is the name of an object as a **user of an application** knows it, in the language the
description is written in. It is UI text, not documentation and not a translation of the type
definition.

```
TYPE amenity_bank
  = NODE AREA ("amenity"=="bank")
    ...
    DESC
      en: "bank"
      de: "Bank"
```

The two places a description is shown today are the result lists of a search and the dialog that
describes an object the user selected on the map. A description answers the question "what is
this?", never "how is this mapped?".

## Rules

### 1. Name the object, not its classification

A description names the thing a user sees or looks for — `bank`, `Bank`, `Autobahn`, `Krankenhaus`.
It does not describe how map data classifies it, and it does not repeat the internal structure of the
configuration.

### 2. Never restate the type identifier

A description must not be the type name, and must not be the type name with its separators replaced
by spaces. `highway_living_street` is not described as `highway living street`; in German it is
`Spielstraße`.

The type identifier stays available to the application as the technical name, so a display that has
no description can fall back to it; a description that equals it adds nothing.

### 3. No tagging vocabulary

Words that belong to map data editing do not belong in a description: the key/value vocabulary of the
data (`way`, `node`, `relation`, `multipolygon`, `key`, `tag`, `value`), the names of the conditions a
type is defined by, and abbreviations used only inside the project.

Where the tag value happens to be the common word in a language (`shop=greengrocer` → "greengrocer"),
the common word is still what is written — the rule is about the user's vocabulary, not about
forbidding a word that appears in the data.

### 4. Each language is worded on its own

A description is written for the language it is in, not translated word for word from another one.
Where a language has its own word for the object, that word is used, even where the other language
uses a longer phrase.

The German description of a type is not derived from the English one, and vice versa.

### 5. German and English are the shipped languages

The shipped type configuration carries German (`de`) and English (`en`). A third language is not
added by guesswork: it is added when somebody can maintain it, and until then the configuration
carries the two.

### 6. Where the name is uncertain, the description stays empty

A wrong name is worse than no name: a user who reads `Spielstraße` for a motorway is misled, while a
user who reads the technical name can still recognise the object or ignore it.

A description is therefore left out, deliberately, when

- the meaning of the type is not established beyond doubt (an unusual or overlapping tag combination),
- the type exists only to classify data and a user never encounters it,
- two types differ in a way that is invisible to a user, so a distinguishing wording would be
  invented, or
- the word for the object in one of the shipped languages is not known.

An omitted description is not a defect to be fixed by a guess. It is recorded with its reason in
`Tests/data/TypeDescriptionOmissions.txt`, and the coverage test
(`Tests/src/TypeDescriptionCoverageTest.cpp`) requires every type to be either described or recorded
there.

The mechanical part of this work — collecting a batch of names, inserting the `DESC` blocks in the
place the parser expects, and proving the coverage — is the `add-type-descriptions` skill
(`.pi/skills/add-type-descriptions/`).

### 7. Where the object has no user-facing name, describe the object, not the type

A type that covers several objects of the same kind takes the wording of the kind a user would name
— the wording of the type's own meaning, not a list of what it includes.

## Syntax of a description

A description is written in the `DESC` block of a definition, one line per language, and the block
comes after everything else that belongs to the definition:

```text
  TYPE amenity_bank
    = NODE AREA ("amenity"=="bank")
      {Name, NameAlt}
      PATH[FOOT] LOCATION
      GROUP amenity, routingPOI
      DESC
        en: "bank"
        de: "Bank"

FEATURES
  FEATURE Width
  DESC
    en: "width"
    de: "Breite"
```

The language code before the colon is what a caller asks for. A definition may carry one language
only; a caller that asks for a language the definition does not carry is answered with the English
description, and a caller that asks for a language a type carries no description at all for is
reported an empty result (`TypeInfo::GetLocalizedDescription` in `libosmscout/TypeConfig.h`). A
type whose user-facing name could not be established carries no block, and its name is recorded in
`Tests/data/TypeDescriptionOmissions.txt`. `Tests/src/TypeDescriptionCoverageTest.cpp` fails on a
definition that has neither.

A parked definition carries its descriptions inside its `PARKED-NEW-TYPE` block, so that enabling
the type brings the description along:

```text
  // PARKED-NEW-TYPE
  // TYPE highway_busway
  //   = WAY ("highway"=="busway")
  //     DESC
  //       en: "busway"
  //       de: "Busfahrbahn"
```

### What a description does to the database

A description is content, not layout: `guidelines/FileFormatVersion.md` lists "groups / descriptions"
among the changes that keep `FILE_FORMAT_VERSION` and only require a re-import. The `DESC` block is
self-describing and count-prefixed in `types.dat`, so an older library build reads a file that
carries new descriptions without a change of its own, and an existing database keeps opening; it
simply carries the descriptions it was generated with, and until it is regenerated the clients of
that database answer with an empty description and show the technical type name.

