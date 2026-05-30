## ADDED Requirements

### Requirement: Libraries install to correct platform libdir

When building with CMake and installing, all shared library targets SHALL be installed to the directory determined by `GNUInstallDirs` `${CMAKE_INSTALL_LIBDIR}` instead of the hardcoded `lib`.

#### Scenario: Shared libraries install to correct libdir

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_PREFIX=/usr`
- **WHEN** the user runs `cmake --install`
- **THEN** shared library files (`.so`) SHALL be installed under `${CMAKE_INSTALL_LIBDIR}` (e.g. `lib64/` on 64-bit Fedora, `lib/x86_64-linux-gnu/` on Debian multiarch, `lib/` on standard systems)

#### Scenario: Default libdir remains unchanged on standard systems

- **GIVEN** a system where `GNUInstallDirs` resolves `CMAKE_INSTALL_LIBDIR` to `lib`
- **WHEN** the user runs `cmake --install`
- **THEN** libraries SHALL install to `lib/`, matching previous behavior exactly

### Requirement: Pkg-config files install to correct libdir

When installing via CMake, `.pc` pkg-config files SHALL be installed under `${CMAKE_INSTALL_LIBDIR}/pkgconfig` instead of hardcoded `lib/pkgconfig`.

#### Scenario: .pc files follow libdir

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_PREFIX=/usr`
- **WHEN** the user runs `cmake --install`
- **THEN** `.pc` files SHALL be installed to `${CMAKE_INSTALL_LIBDIR}/pkgconfig`

### Requirement: CMake config files install to correct libdir

The CMake package config files (`libosmscoutConfig.cmake`, `libosmscoutConfigVersion.cmake`) SHALL be installed under `${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}` instead of hardcoded `lib/cmake/${PROJECT_NAME}`.

#### Scenario: Config files follow libdir

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_PREFIX=/usr`
- **WHEN** the user runs `cmake --install`
- **THEN** CMake config files SHALL be installed to `${CMAKE_INSTALL_LIBDIR}/cmake/libosmscout`

### Requirement: Executables install to correct bindir

Executable targets SHALL use `${CMAKE_INSTALL_BINDIR}` instead of hardcoded `bin` for consistency (though the default value is `bin` on virtually all platforms).

#### Scenario: Executables install to bindir

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_PREFIX=/usr`
- **WHEN** the user runs `cmake --install`
- **THEN** executables SHALL be installed to `${CMAKE_INSTALL_BINDIR}`

### Requirement: GNUInstallDirs included once at project root

The `GNUInstallDirs` module SHALL be included exactly once, in the root `CMakeLists.txt`, positioned after the `project()` call and before any `install()` call or `add_subdirectory()` that triggers one.

#### Scenario: Variable available in all subdirectories

- **GIVEN** the root `CMakeLists.txt` includes `GNUInstallDirs`
- **WHEN** any subdirectory's `CMakeLists.txt` or included cmake file references `${CMAKE_INSTALL_LIBDIR}`
- **THEN** the variable SHALL be defined with the correct platform value
