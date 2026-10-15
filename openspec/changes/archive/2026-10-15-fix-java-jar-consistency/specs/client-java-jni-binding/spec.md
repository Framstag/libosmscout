# Spec Delta

## Purpose

The contract between the Java client's `native` declarations and the JNI functions that implement them: parameter types agree, and the magnification that crosses the boundary is the level the Java API documents, validated before it reaches a projection.

## ADDED Requirements

### Requirement: Native parameter lists match the Java declarations

Every `native` method declaration of the Java client and the JNI function that implements it SHALL agree on the parameter types, in order and in count, so that no parameter is read from the wrong place. The agreement SHALL be checkable by comparing the two source files on a host, without a database, a device or a rendering run.

#### Scenario: A declaration and its implementation agree

- **GIVEN** the Java declarations and the JNI functions of the client
- **WHEN** their parameter lists are compared
- **THEN** every method has the same parameter types on both sides

#### Scenario: A changed parameter type is detectable

- **GIVEN** a parameter whose type differs between the Java declaration and the JNI function (for example an integer level declared in Java and read as a floating-point scale in C++)
- **WHEN** the two parameter lists are compared
- **THEN** the difference is reported for that method, naming both type lists
- **AND** the comparison needs no database, device or rendering run

### Requirement: The magnification crosses the boundary as a validated level

The render and projection entry points of the client SHALL take the magnification as the level the Java declarations document (an integer, `0` = world), not as a scale factor. A level outside the range the library's cell dimension table defines SHALL NOT reach a projection: the call SHALL produce no result and report a warning, with the same behaviour in a build with and without asserts.

#### Scenario: A level in range renders

- **GIVEN** a client with an open database
- **WHEN** rendering is requested with a magnification level within the supported range
- **THEN** a render result is produced

#### Scenario: A level above the supported range is refused

- **GIVEN** a magnification level above the highest level the cell dimension table defines
- **WHEN** a render or projection call is made
- **THEN** the call produces no result
- **AND** a warning naming the level and the supported range is reported
- **AND** the process does not abort, whether or not the library was built with asserts

#### Scenario: A negative level is refused

- **GIVEN** a negative magnification level
- **WHEN** a render or projection call is made
- **THEN** the call produces no result and reports the supported range
