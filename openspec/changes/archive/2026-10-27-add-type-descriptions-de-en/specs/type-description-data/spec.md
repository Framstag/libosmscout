# Spec Delta

## Purpose

Gives the shipped type configuration localized, user-facing descriptions of its types and features,
so that an application can name what an object is in the language of its user instead of showing a
technical type identifier.

## ADDED Requirements

### Requirement: Shipped type configuration describes its types

Every user-facing type definition in the shipped type configuration SHALL carry a description in the
shipped languages, whether the definition is active or retained in inactive form, except where the
description is deliberately left out under the uncertainty rule of this capability.

#### Scenario: An active type carries descriptions

- **WHEN** the shipped type configuration is loaded
- **THEN** a type that a user can encounter in an application SHALL expose a description in every shipped language

#### Scenario: An inactive type carries descriptions

- **WHEN** a type definition retained in inactive form is enabled
- **THEN** it SHALL expose the same descriptions as an active definition, without an additional edit

#### Scenario: A type whose name is uncertain carries none

- **WHEN** the user-facing name of a type could not be established
- **THEN** the shipped type configuration SHALL carry no description for that type

### Requirement: Descriptions are worded for application users

A description SHALL be the name that a user of an application recognizes for the described object,
and SHALL NOT be the type identifier, the type identifier restated with separators replaced by
spaces, or vocabulary that belongs only to map data editing.

#### Scenario: A type identifier is not a description

- **WHEN** a type's description is read
- **THEN** it SHALL differ from the type identifier and from the type identifier with its separators replaced

#### Scenario: Tagging vocabulary is not used

- **WHEN** a description names an object whose common user-facing name differs from the vocabulary used in map data
- **THEN** it SHALL use the common user-facing name

#### Scenario: A type that a user never sees may carry none

- **WHEN** a type exists only so that map data can be classified and a user never encounters it
- **THEN** the shipped type configuration MAY carry no description for that type

### Requirement: Each shipped language is worded on its own

A description SHALL be written for the language it is in, and SHALL NOT be a word-for-word rendering
of another language's description.

#### Scenario: Descriptions in two languages differ in wording

- **WHEN** a type carries a description in two shipped languages
- **THEN** each SHALL read as the wording that language uses for the described object, including where that wording is not a translation of the other

### Requirement: The shipped languages are a closed set

The shipped type configuration SHALL carry descriptions in German and in English, and SHALL NOT
carry descriptions in a further language. A type MAY carry only one of the two, in which case the
missing one is absent rather than approximated.

#### Scenario: Both shipped languages are present

- **WHEN** a described type is read
- **THEN** its German and its English description SHALL both be available

#### Scenario: One shipped language may be absent

- **WHEN** a described type carries only one of the two shipped languages
- **THEN** the other SHALL be reported as absent for that type, and it SHALL NOT be filled from the present one

### Requirement: Features carry descriptions as well

A feature of the shipped type configuration that a user can encounter SHALL carry descriptions in
the shipped languages, on the same terms as a type.

#### Scenario: A user-facing feature is described

- **WHEN** the shipped type configuration is loaded
- **THEN** a feature whose value an application shows to a user SHALL expose descriptions in the shipped languages

#### Scenario: A feature already described keeps its description

- **WHEN** a feature carried descriptions before this change
- **THEN** it SHALL still carry a description in each shipped language

### Requirement: A missing description is a reported state

The shipped type configuration SHALL let a caller distinguish a type that carries no description
from a type that carries a description, without the caller comparing the description to the type
identifier.

#### Scenario: A caller recognizes an undescribed type

- **WHEN** a caller reads the description of a type that carries none
- **THEN** the caller SHALL receive an outcome that no description is present
