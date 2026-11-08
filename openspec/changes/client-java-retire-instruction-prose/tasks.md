# Tasks

## 1. Bridge
- [x] Remove the description walk and the `RouteInstruction`/`TurnType` JNI projection from `OSMScoutClient.cpp`
- [x] Publish `RouteEntry.distance` from the last step's cumulative distance (`LastStepDistanceMeters`)

## 2. Java API
- [x] Drop the five prose fields from `RouteEntry.java`, keep `steps`
- [x] Delete `RouteInstruction.java`
- [x] Remove `RouteInstruction.java` from the CMake and Meson Java source lists

## 3. JavaScout and docs
- [x] Drop the description-line assertions from `OSMScoutClientRouteStepTest.java`
- [x] Update the `RouteInstruction` rows in `libosmscout-client-java/AGENTS.md` and `JavaScout/AGENTS.md`

## 4. Verification
- [x] Build the JNI translation unit and the jar
- [x] Compile the JavaScout sources against the new API
- [x] Run the host step tests and the JNI signature check
- [x] `openspec validate client-java-retire-instruction-prose --strict`
