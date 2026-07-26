# JavaScout

JavaFX desktop application for libosmscout. Uses the Java client library (`libosmscout-client-java`) to open and display OSM map databases.

## Prerequisites

- Java 17+
- Apache Maven 3.8+ (or use the `mvnw` wrapper)
- libosmscout native libraries (CMake or Meson)

## Building

### 1. Build native libraries

#### Option A: CMake

The Java client library now has a `CMakeLists.txt`. Build everything in one go:

```bash
cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON
cmake --build build
sudo cmake --install build
```

#### Option B: Meson

```bash
meson setup build -Dbuild_java=true
meson compile -C build
```

### 2. Build JavaScout (Maven)

JavaScout depends on the `libosmscoutclientjava.jar` produced by the C++ build.
`build.sh` locates the jar (from either CMake or Meson build directories),
installs it into your local Maven repository, and packages JavaScout.

```bash
cd JavaScout
./build.sh
```

If the jar is in a non-standard build directory, pass it explicitly:

```bash
./build.sh /absolute/path/to/libosmscoutclientjava.jar
```

> **Note:** After changing any `libosmscout-client-java` Java sources, rebuild the
> client jar and run `./build.sh` again so the local Maven repository stays in sync.

## Running

JavaScout no longer requires a local maps directory argument. Maps are downloaded
from the configured provider inside the app and stored in the default download
directory (`~/.config/javascout/maps/` on Linux).

```bash
./javascout.sh
```

If no maps exist yet, the map view starts empty and the placeholder shows
"No map loaded — download a map via the menu".

Additional options:

```bash
./javascout.sh --stylesheet-dir /path/to/stylesheets --icon-dir /path/to/icons --map-provider karry.cz
```

### With Maven (development)

```bash
cd JavaScout
mvn javafx:run
```

Or with a specific maps directory:

```bash
cd JavaScout
mvn javafx:run -Djavafx.args=/path/to/maps/directory
```

### With fat jar (easiest — includes client library)

```bash
cd JavaScout
./javascout.sh /path/to/maps/directory
```

The script auto-detects the build directory and uses the fat jar.

### Direct java command

```bash
java --enable-native-access=ALL-UNNAMED \
     -Djava.library.path=../build/libosmscout-client-java/src \
     -jar target/javascout-1.0-SNAPSHOT.jar \
     /path/to/maps/directory
```

## Configuration

JavaScout stores its configuration in an OS-specific location:

| Platform | Config path |
|----------|-------------|
| Linux    | `~/.config/javascout/config.properties` |
| macOS    | `~/Library/Application Support/JavaScout/config.properties` |
| Windows  | `%APPDATA%\JavaScout\config.properties` |

### Config file format

```properties
# JavaScout configuration
maps.directory=/home/user/maps
```

When you run JavaScout with a CLI argument, the path is automatically saved to the config file. On subsequent runs without an argument, the saved path is used.

## Usage

```
javascout.sh                    # Opens with previously configured maps directory
javascout.sh /path/to/maps      # Opens with specified directory, saves for next run
```

The application scans the configured directory for `.osmscout` map databases and loads all found databases automatically.
