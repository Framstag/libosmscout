# Verification

Ported from reference commit `fab852ff9` ("libosmscout-client-java: the legacy route
entry logs identity, not a position") onto the current `origin/master`.

## Commands and results

### Source: no coordinate left in the legacy route entry

```
$ git grep -n "GetDisplayText" -- libosmscout-client-java/src/OSMScoutClient.cpp
(no output; exit code 1)
```

Before the change the same command listed seven statements, all replaced by this unit
(`6855` box check, `7584`/`7593` lookup successes, `8761`/`8769` start, `8779`/`8787`
destination). No `GetDisplayText` use remains in the file.

### Diff shape

```
$ git diff --stat
 libosmscout-client-java/src/OSMScoutClient.cpp | 20 +++++++-------------
 1 file changed, 7 insertions(+), 13 deletions(-)
```

Matches the reference unit: 7 insertions / 13 deletions.

### Build of the exact target

```
$ cmake --build build --target osmscout_client_java
...
[5/7] Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/Unity/unity_0_cxx.cxx.o
[6/7] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so
```

Compiles and links. Only the pre-existing CMake `FindPackageHandleStandardArgs` author
warnings (Glib2/GObject/Pangocairo/Pangoft2) appear; no compiler warning from the
changed file.

### JNI signature check

```
$ bash scripts/check-jni-signatures.sh
WARNING: JNI function without a Java declaration: getMaxSpeedAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: reloadBasemap ()
WARNING: JNI function without a Java declaration: getAddressAt (jdouble,jdouble)
WARNING: JNI function without a Java declaration: getDatabaseBoundingBox (jstring)
WARNING: JNI function without a Java declaration: searchLocationByForm (jstring,jstring,jstring,jstring,jint,jlong)
JNI signatures match: 60 native declarations checked, 5 dead JNI functions reported above.
```

Signatures match. The five warnings are pre-existing (no declaration was changed here).

### Openspec

```
$ openspec validate client-java-jni-binding-log-identity --strict
Change 'client-java-jni-binding-log-identity' is valid
```

## What could not be run and why

- **The Java route path.** No runtime exercise of `calculateRouteAsync` is possible on
  this host: the map database under `maps/` is file-format v26 while this library reads
  v27, so there is no database the route entry can open. No database was imported for
  this change. The change is confined to log lines, so the source check above is the
  verification; the JavaScout suite was not run.
- **`scripts/format-check.sh`.** Red repository-wide; not used. The edits match the
  surrounding style (statement indentation and line wrapping).
