## ADDED Requirements

### Requirement: Meson CI workflow file

The workflow file `.github/workflows/build_javascout_meson.yml` SHALL build the minimal C++ subset with Meson and then build JavaScout with Maven.

#### Scenario: JDK setup via setup-java
- **WHEN** the job runs
- **THEN** `actions/setup-java@v5` installs JDK 25 temurin with Maven cache

#### Scenario: Minimal C++ deps installed
- **WHEN** the job runs on ubuntu-24.04
- **THEN** only required deps are installed: libxml2, liblzma, libpng, libcairo, libpango, freetype, protobuf, python3-pip (for meson)

#### Scenario: Meson configured with minimal features
- **WHEN** running `meson setup build -Dbuild_java=true -DenableImport=false -DenableGpx=false -DenableMapAgg=false -DenableMapCairo=true -DenableMapOpenGL=false -DenableMapQt=false -DenableMapSvg=false -DenableClientQt=false -DenableTests=false -DbuildDemos=false`
- **THEN** configuration succeeds with only core, map, cairo, client, client-java enabled

#### Scenario: Full Meson build
- **WHEN** running `ninja -C build`
- **THEN** all C++ libraries and `libosmscoutclientjava.jar` are built

#### Scenario: Jar installed to local Maven repo
- **WHEN** running `mvn install:install-file -Dfile=build/libosmscout-client-java/java/libosmscoutclientjava.jar -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar -q`
- **THEN** the jar is available in the local Maven repository

#### Scenario: JavaScout Maven package
- **WHEN** running `mvn -f JavaScout/pom.xml package -q`
- **THEN** JavaScout fat jar is produced at `JavaScout/target/javascout-1.0-SNAPSHOT.jar`

#### Scenario: JavaScout unit tests pass
- **WHEN** running `mvn -f JavaScout/pom.xml test`
- **THEN** all unit tests pass
