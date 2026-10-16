# source-include-order Specification

## Purpose
The include block of a checked source file names its dependencies in the order the project's code style
prescribes: the file's own header first, then the standard library, then the project's headers
(`guidelines/CodeStyles.md`, "Include Order"). The order is a property of the file's text, so a host check
reads the file instead of running the program.

## Requirements

### Requirement: Standard library includes precede project includes

`libosmscout/src/osmscout/util/StringMatcher.cpp` SHALL NOT include a standard library header after its
project includes; a project include here is any `osmscout` header other than the file's own header. The check
SHALL read the file under `SOURCE_ROOT`, report every such include and the number of them, and fail while the
number is greater than zero.

#### Scenario: The checked file lists no standard library include after its project headers

- **GIVEN** the source tree at `SOURCE_ROOT`
- **WHEN** the case "Check include order of util/StringMatcher.cpp" of `HeaderCheckTest` runs
- **THEN** the number of standard library includes that follow a project include other than the file's own
  header SHALL be 0
- **AND** the case SHALL print that number
