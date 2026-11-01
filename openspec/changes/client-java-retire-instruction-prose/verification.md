# Verification

## Commands run

| command | result |
|---|---|
| `cmake --build build --target osmscout_client_java` | JNI translation unit compiles and links (`[3/4] Linking … libosmscout_client_java.so`) |
| `cmake --build build --target java_jar` | `[1/1] Packaging Java classes into jar` |
| `mvn -o -q -DskipTests -f JavaScout/pom.xml test-compile` | JavaScout Java sources and tests compile against the retimed API (no `RouteInstruction`) |
| `ctest --test-dir build -R RouteStep --output-on-failure` | `100% tests passed out of 2` (`RouteStepTimeTest`, `RouteStepModelTest`) |
| `bash scripts/check-jni-signatures.sh` | `JNI signatures match: 60 native declarations checked` (5 pre-existing dead JNI functions, unrelated) |
| `openspec validate client-java-retire-instruction-prose --strict` | `Change 'client-java-retire-instruction-prose' is valid` |
| `git grep -n "RouteInstruction\|instructionLats\|instructionDistances\|instructionTimes\|routeDescriptionLines"` over `JavaScout/` and `libosmscout-client-java/` | only the unrelated C++ `RouteInstructionAgent`/`RouteInstructionsMessage` core symbols remain |

## Not run

- The end-to-end Java route path and the JavaScout navigation tests: they need a routable map database, and the
  databases in `maps/` are data-format v26 while this build reads v27. No database was imported. The change is
  compile-verified on the JNI translation unit and the JavaScout sources, plus the host step tests above.
