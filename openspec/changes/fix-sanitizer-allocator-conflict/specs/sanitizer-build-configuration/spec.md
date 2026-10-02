# Spec Delta

## Purpose

Defines what a build configuration that enables a sanitizer runtime guarantees about the allocator its
binaries use, so that a run of one of those binaries reports the program's own memory errors instead of
a conflict between two allocator interposers, and so that the heap profiling of the performance tool
stays available where it is safe.

## ADDED Requirements

### Requirement: A sanitizer configuration provides one allocator interposer

A build configuration that enables a sanitizer runtime SHALL NOT link a second allocator interposer
into the binaries it builds. A binary of such a configuration SHALL use the sanitizer runtime's
allocator, so that a memory error it reports is the program's own.

#### Scenario: The performance tool of a sanitizer configuration links no second interposer

- **GIVEN** a configuration whose compiler and linker flags enable a sanitizer runtime
- **WHEN** the performance tool is built
- **THEN** the dynamic dependencies of the built binary SHALL NOT contain a heap allocator other than the sanitizer runtime's

#### Scenario: A manual run of that binary reports no invalid free

- **GIVEN** the built performance tool of that configuration
- **WHEN** it renders the fixed view of the Dortmund database through the OpenGL driver at 300 DPI
- **THEN** the process SHALL exit without reporting an invalid free
- **AND** it SHALL report the same loaded objects and rendering counters as the same view rendered by an ordinary configuration

#### Scenario: A configuration that enables a sanitizer says which interposer it dropped

- **GIVEN** a configuration whose flags enable a sanitizer runtime and whose heap profiler option is on by default
- **WHEN** the project is configured
- **THEN** the configuration output SHALL name the heap profiler as disabled and SHALL name the sanitizer runtime as the reason

### Requirement: The heap profiler stays available outside sanitizer configurations

A configuration that enables no sanitizer runtime SHALL keep the heap profiler of the performance tool
available under the option it already offers, so the change does not remove a developer feature.

#### Scenario: An ordinary configuration still links the profiler

- **GIVEN** a configuration without a sanitizer runtime whose heap profiler option is on
- **WHEN** the performance tool is built
- **THEN** the built binary SHALL link the profiler library

#### Scenario: The profiler can still be turned off explicitly

- **GIVEN** a configuration without a sanitizer runtime and with the heap profiler option turned off
- **WHEN** the project is configured
- **THEN** the performance tool SHALL be built without the profiler library
