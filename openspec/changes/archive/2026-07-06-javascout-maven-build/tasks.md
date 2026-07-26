## 1. Create JavaScout/pom.xml

- [x] 1.1 Create `JavaScout/pom.xml` with project metadata (groupId, artifactId, version, packaging) (est: 1)
- [x] 1.2 Add `maven-compiler-plugin` config targeting Java 17+ (est: 1)
- [x] 1.3 Add `maven-jar-plugin` with `mainClass` set to `com.framstag.libosmscout.JavaScout` (est: 1)
- [x] 1.4 Use `system` scope with `systemPath` instead of `install:install-file` (avoids dependency resolution timing issue) (est: 1)
- [x] 1.5 Add `libosmscout-client-java` dependency with `compile` scope (est: 1)
- [x] 1.6 Add `client-java.jar` property with default path `../build/java/libosmscoutclientjava.jar` (est: 1)
- [x] 1.7 Remove env var profile (Maven can't resolve relative paths from env vars for systemPath) (est: 1)

## 2. Update root meson.build

- [x] 2.1 Remove or comment out `subdir('JavaScout')` in root `meson.build` (est: 1)
- [x] 2.2 Add comment explaining JavaScout now uses Maven (est: 1)

## 3. Remove or archive JavaScout/meson.build

- [x] 3.1 Delete `JavaScout/meson.build` (est: 1)

## 4. Add build documentation

- [x] 4.1 Create `JavaScout/README.md` with build and run instructions (est: 2)
- [x] 4.2 Document `CLIENT_JAVA_JAR` env var usage (est: 1)
- [x] 4.3 Document runtime native library path setup (est: 1)

## 5. Create launcher script (optional)

- [x] 5.1 Create `JavaScout/javascout.sh` that detects build dir and runs JavaScout with correct classpath and library path (est: 2)
- [x] 5.2 Add `maven-assembly-plugin` for fat jar (`jar-with-dependencies`) (est: 1)
- [x] 5.3 Update launcher to prefer fat jar, fall back to thin jar + classpath (est: 1)

## 6. Update CI (if applicable)

- [x] 6.1 Check if any CI workflow builds JavaScout and update to use Maven (est: 2)
