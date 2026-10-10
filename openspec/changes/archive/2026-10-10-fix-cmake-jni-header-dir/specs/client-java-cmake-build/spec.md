# Spec Delta

## MODIFIED Requirements

### Requirement: JNI header generation

The CMake build SHALL generate JNI native headers from the Java source files before compiling the C++ library. The generated header directory SHALL contain the headers of the Java sources present when the generation ran and no header of a source that is no longer present, so that a removed or renamed native method cannot leave a stale header for the C++ build to include.

#### Scenario: Headers generated before C++ compile
- **WHEN** running `cmake --build`
- **THEN** `javac -h` runs on all Java sources under `java/` and headers are written to a known include directory

#### Scenario: All Java sources produce headers
- **WHEN** checking the generated header directory
- **THEN** one header per class that declares a native method is present — for the current sources
  `MapDownloadManager`, `NavigationController`, `OSMScoutClientBuilder` and `OSMScoutClient` — because
  `javac -h` emits a header only for a class that declares one

#### Scenario: A header of a removed source is not kept
- **GIVEN** a build tree whose JNI header directory holds a header that corresponds to no Java source of the current tree
- **WHEN** the JNI header generation runs again
- **THEN** that header is gone afterwards and the directory holds the headers of the current sources
