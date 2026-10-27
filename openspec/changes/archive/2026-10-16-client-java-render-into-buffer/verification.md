# Verification

Base branch `origin/client-java-render-dpi` (`db57e106`), branch `client-java-render-into-buffer`.
All commands run in `/home/tim/projects/libosmscout` unless the working directory is changed.

## 1. CMake build and the new host test

- `cmake --build build --target FramePixelLayoutTest` — reconfigures, compiles and links the new test:
  `[1/3] Building CXX object Tests/CMakeFiles/FramePixelLayoutTest.dir/src/FramePixelLayoutTest.cpp.o`,
  `[2/3] Linking CXX executable Tests/FramePixelLayoutTest-1.1.1`. No warning from the new file.
- `cd build && ctest -R FramePixelLayoutTest --output-on-failure` — passed; the verbose run reports
  `All tests passed (35 assertions in 4 test cases)`.
- `cmake --build build --target osmscout_client_java` — `[3/3] Linking CXX shared library
  libosmscout-client-java/libosmscout_client_java.so`; `OSMScoutClient.cpp` compiles without a warning.
- `cmake --build build --target java_jar` — `[1/1] Packaging Java classes into jar`, so the changed
  `OSMScoutClient.java` is compiled by `javac` (not only parsed).

## 2. JNI signature parity

- `bash scripts/check-jni-signatures.sh` — exit 0:
  `JNI signatures match: 56 native declarations checked, 5 dead JNI functions reported above.`
  The 5 dead functions (`getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`, `getAddressAt`,
  `getDatabaseBoundingBox`) are present before this change and none of them is `renderInto`: the new native
  declaration and its `Java_..._renderInto` function have the same argument list.

## 3. C++ regression suite

- `cd build && ctest -j 2 --exclude-regex "PerformanceTest"` — `100% tests passed out of 107`. This includes
  `FramePixelLayoutTest`, `SearchScopeTest` and `JniSignatureParityTest`, the checks nearest to the change.
  No existing test regresses. The change touches only the JNI shared library, which no host test links, and
  a new test binary.

## 4. Meson build and tests

- `meson setup --reconfigure build-meson` — reconfigured to pick up the new test target.
- `meson compile -C build-meson FramePixelLayoutTest osmscout_client_java libosmscoutclientjava` — builds the
  test, the native shared library and the JAR with no error (the 69 javadoc warnings are the repository's
  pre-existing ones).
- `meson test -C build-meson --list` — lists `Check frame pixel layout` and `Check JNI signature parity`.
- `meson test -C build-meson "Check frame pixel layout" "Check JNI signature parity"` — `Ok: 2 / Fail: 0`.

## 5. The layout test has teeth

Temporarily swapping the channel order of the `ArgbInt` branch of `FrameDestination::Write` (writing red and
blue exchanged), rebuilding `FramePixelLayoutTest` and running it:

```
ctest -R FramePixelLayoutTest --output-on-failure
FramePixelLayoutTest ...***Failed
Tests/src/FramePixelLayoutTest.cpp:81: FAILED    (the int[] word is 0xFFF57A7Du, not 0xFF7D7AF5u)
Tests/src/FramePixelLayoutTest.cpp:142: FAILED   (the same colour reads back with r and b swapped)
Tests/src/FramePixelLayoutTest.cpp:163: FAILED   (the buffer word is no longer the byte-swap of the array word)
test cases:  4 |  1 passed | 3 failed
assertions: 21 | 18 passed | 3 failed
```

The swap was reverted and the test passes again (`100% tests passed out of 1`). The seam case and the
byte-order cases therefore fail when one destination is written in the other's layout.

## 6. OpenSpec

- `openspec validate client-java-render-into-buffer --strict` — `Change 'client-java-render-into-buffer' is
  valid`.

## 7. What is not verified here (task 5.5)

The JNI part of the buffer entry point — `GetDirectBufferAddress` / `GetDirectBufferCapacity`, the
precondition checks and the ownership behaviour — is not host-testable. It needs a real Android/desktop
surface that consumes a direct buffer, and it needs a render, which needs a usable map database; the
databases in `maps/` are v26 while the library expects v27 and cannot be opened. No map-dependent test was
invented. The pixel contract the JNI part feeds is the host-tested layout: `FramePixelLayoutTest` pins the
`RgbaBytes` byte order and the agreement of the two destinations, which is the failing part of the channel
swap. Task 5.5 was unticked for that reason until 2026-10-16.

## Host verification 2026-10-16 (map-dependent items)

Task 5.5 is now ticked: the host surface the task allows was used, against the v27 database
`maps/nordrhein-westfalen`.

```bash
# compiled once into build/loop-probes-2026-10-16
java -Djava.library.path=build/libosmscout-client-java \
  -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/loop-probes-2026-10-16 \
  BufferColorProbe maps/nordrhein-westfalen stylesheets
```

The probe renders the same view (64x48 at 51.514/7.465, magnification 32768) twice, both with the same
3-waypoint route overlay: once through `renderWithRouteAndPois` (int[] ARGB, 3072 elements) and once through
`renderInto` (direct `ByteBuffer`, capacity 12288 bytes). It compares every pixel channel for channel —
the array word `0xAARRGGBB` against the buffer bytes `R,G,B,A`.

| probe | result |
|---|---|
| `ARRAY_PIXELS` / `BUFFER_CAPACITY` | `3072` / `12288` |
| `NON_TRANSPARENT` | `3072` |
| `PIXELS_DIFFERING_ARRAY_VS_BUFFER` | `0/3072` |

Read: the buffer path shows exactly the colours of the array path on a real map, so the channel-swap
regression is not present. **What stayed unverified:** a real Android/desktop surface that consumes the
direct buffer — the `GetDirectBufferAddress` / ownership plumbing of a frame that is never copied back to
an `int[]`.

Log: `evidence/buffer-color-probe-nrw-2026-10-16.log` (command, UTC timestamp, exit 0, revision `06b197a35`).
Probe source: `evidence/BufferColorProbe.java`.
