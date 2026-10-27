# type-description-lookup Specification

## Purpose
Lets an application ask the loaded type configuration what a type is called in the language of its
user, and receive a defined answer for a language the configuration does not describe.

## Requirements

### Requirement: A description is resolved by language

A caller SHALL be able to obtain the description of a type for a language it names, and SHALL
receive the description the configuration carries for that language.

#### Scenario: The requested language is described

- **WHEN** a caller asks for the description of a type in a language the configuration describes
- **THEN** it SHALL receive the description carried for that language

#### Scenario: The type is unknown

- **WHEN** a caller asks for the description of a type that is not part of the loaded configuration
- **THEN** it SHALL be reported that no description is available, and no error SHALL be raised

### Requirement: The absence of a description is distinguishable

A resolution SHALL report that no description is present when the configuration carries none for the
requested language, and SHALL NOT return the type identifier in its place.

#### Scenario: No description for the requested language

- **WHEN** a caller asks for a description in a language the configuration does not carry for that type
- **THEN** it SHALL be reported that no description is present

#### Scenario: The English description answers a missing language

- **WHEN** a caller asks for a description in a shipped language the configuration does not carry, and the configuration carries the English description of that type
- **THEN** the English description SHALL be returned

#### Scenario: No shipped language is described

- **WHEN** a caller asks for a description of a type the configuration carries no description for
- **THEN** it SHALL be reported that no description is present, for every requested language

### Requirement: A caller can obtain every description of a type

A caller SHALL be able to obtain all descriptions a type carries, keyed by language, without asking
for one language at a time.

#### Scenario: All descriptions are listed

- **WHEN** a caller asks for the descriptions of a type
- **THEN** it SHALL receive each described language with its description

#### Scenario: An undescribed type lists nothing

- **WHEN** a caller asks for the descriptions of a type that carries none
- **THEN** it SHALL receive an empty result

### Requirement: The shipped bindings expose descriptions

A binding that reads the type configuration SHALL make the descriptions of the loaded configuration
available to its callers, on the same terms as the native lookup.

#### Scenario: Java bindings expose both lookup forms

- **WHEN** a Java caller holds a type of the loaded configuration
- **THEN** it SHALL be able to obtain one description by language and all descriptions at once

#### Scenario: A search result reports its object's description

- **WHEN** a client bridge returns a search result for an object
- **THEN** the result SHALL report the description of that object's type for the language the caller asked for

### Requirement: Runtime-registered types have no descriptions

A type registered by an application at runtime, which is not part of a type configuration read from a
database, SHALL be reported as carrying no description, for every requested language.

#### Scenario: A synthetic type is undescribed

- **WHEN** a caller asks for the description of a type an application registered at runtime
- **THEN** it SHALL be reported that no description is present

### Requirement: A configuration without descriptions still loads

A type configuration that carries no descriptions SHALL load without error, and every type of it
SHALL be reported as carrying no description, so that a database generated before this change keeps
working and an application shows what it showed before.

#### Scenario: An older database loads unchanged

- **WHEN** a database whose type configuration carries no descriptions is opened
- **THEN** it SHALL load without error
- **AND** every type SHALL be reported as carrying no description
