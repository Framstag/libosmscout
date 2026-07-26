## ADDED Requirements

### Requirement: Meson subproject build

The build system SHALL compile `libosmscout-client-java` as a Meson subproject producing:
- A shared library `libosmscout_client_java.so` (Linux) linked against `libosmscout`, `libosmscout-map`, `libosmscout-client`
- A JAR archive `libosmscoutclientjava.jar` containing the compiled Java class

> **Note**: `libosmscout-binding` (SWIG-generated) is a separate, independent library. This change builds `libosmscout-client-java` from scratch with hand-written JNI — no reuse of or dependency on `libosmscout-binding`.
The subproject SHALL be conditionally built only when `buildJava` option is `true` in the root `meson.build`.

#### Scenario: Build with Java enabled

- **WHEN** `meson setup build -Dbuild_java=true` is configured
- **AND** `meson compile -C build` is run
- **THEN** `libosmscout_client_java.so` SHALL exist in the build output
- **AND** `libosmscoutclientjava.jar` SHALL exist in the build output

#### Scenario: Build without Java is skipped

- **WHEN** `meson setup build` is configured without `-Dbuild_java=true`
- **THEN** the `libosmscout-client-java` subproject SHALL NOT be built
- **AND** no `libosmscout_client_java.so` or `libosmscoutclientjava.jar` SHALL be produced

### Requirement: JNI header generation

The build SHALL auto-generate JNI native headers from `OSMScoutClient.java` using Meson's `native_headers()` function.

#### Scenario: JNI headers are generated

- **WHEN** the build compiles `OSMScoutClient.cpp`
- **THEN** the JNI header `com_framstag_libosmscout_client_OSMScoutClient.h` SHALL be generated
- **AND** it SHALL be included in the compile step

### Requirement: Export visibility macros

The library SHALL provide a `ClientJavaImportExport.h` header with `OSMSCOUT_CLIENT_JAVA_API` visibility macro for shared library symbol export, following the pattern of `ClientQtImportExport.h`.

#### Scenario: Symbols are exported

- **WHEN** the shared library is built with `default_library=shared`
- **THEN** the `OSMScoutClient JNI functions` SHALL be visible in the dynamic symbol table
- **AND** internal symbols SHALL be hidden (GCC `-fvisibility=hidden`)