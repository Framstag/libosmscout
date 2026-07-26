## Context

`JavaNavigationController` (C++ JNI layer in `libosmscout-client-java`) sets up a `NavigationEngine` with agents for position, bearing, route-state, arrival-estimate, speed, lanes, and voice — but not `RouteInstructionAgent`. The Qt `NavigationModule` has it and produces turn-by-turn `RouteStep` objects. The Java client library needs the same capability so all JavaFX apps (JavaScout, others) can show turn-by-turn guidance.

The `RouteInstructionAgent` is a template parameterized on instruction type and builder:

```cpp
template <typename RouteInstruction, typename RouteInstructionBuilder>
class RouteInstructionAgent : public NavigationAgent { ... };
```

The builder walks `RouteDescription::Node` iterators and produces instruction lists. The agent emits two message types:
- `RouteInstructionsMessage<RouteInstruction>` — full instruction list (on route change)
- `NextRouteInstructionsMessage<RouteInstruction>` — single next instruction (every position update)

Both message types are bridged through JNI. The app primarily uses the live next-instruction updates, but the full list is available for route overview UI.

## Goals / Non-Goals

**Goals:**
- Add `RouteInstructionAgent` to the Java navigation engine
- Bridge instruction messages through JNI to Java
- Show next turn in JavaScout UI (distance, turn type, street name)
- Keep the Java `RouteInstruction` class minimal — only fields the UI needs

**Non-Goals:**
- Voice instruction changes (already works via `VoiceInstructionAgent`)
- Reroute logic (separate concern, has its own TODO)
- Lane guidance changes (already works)

## Decisions

### Decision 1: Java-specific instruction types vs reusing Qt types

**Chosen:** New lightweight C++ `JavaRouteInstruction` struct + `JavaRouteInstructionBuilder` in the JNI layer.

**Alternatives considered:**
- Reuse `RouteStep` / `RouteDescriptionBuilder` from `libosmscout-client-qt` — rejected because it pulls in Qt dependencies (QString, QList) into the Java JNI build
- Build instructions entirely on the Java side by walking `RouteDescription` nodes — rejected because the node iterator and description types are C++ objects not exposed through JNI

### Decision 2: What fields to bridge

**Chosen:** Minimal set: `distanceTo`, `turnType` (enum), `streetName`, `description`, `shortDescription`.

The Qt `RouteStep` has ~15 fields. For a next-turn display we only need 5. Additional fields (destinations, lanes, etc.) can be added later without breaking changes since `NavigationListener` uses default methods.

### Decision 3: How to represent turn type in Java

**Chosen:** String enum names from `RouteDescription::DirectionDescription::Move` serialized via a helper function, matched by a Java enum `TurnType`.

The JNI layer already uses this pattern for `NavigationState` (C++ enum → string → Java `Enum.valueOf()`). Consistent approach.

### Decision 4: Single callback vs separate callbacks

**Chosen:** Two callbacks matching the two message types:
- `onRouteInstructions(RouteInstruction[])` — full list (infrequent, on route change)
- `onNextRouteInstruction(RouteInstruction)` — current next turn (frequent, every position update)

The Qt client uses the same split. The UI primarily cares about `onNextRouteInstruction`; the full list is available for a route overview panel.

## Data Flow

```
GPS fix → JavaNavigationController.ProcessLocation()
              ↓
         engine.Process(GPSUpdateMessage)
              ↓
         RouteInstructionAgent.Process(PositionMessage)
              ↓
         RouteInstructionsMessage / NextRouteInstructionsMessage
              ↓
         DispatchMessage()
              ↓  JNI CallVoidMethod
         NavigationListener
            ├── .onNextRouteInstruction()
            │       ↓  Platform.runLater()
            │   MainController.updateNextTurnOverlay()
            │       ↓
            │   nextTurnBox VBox (separate overlay, top-left of mapPanel)
            │
            └── .onRouteInstructions()
                    ↓  Platform.runLater()
                MainController → RoutePanel.updateInstructionList()
                    ↓
                instructionScroll inside RoutePanel (bottom-right)
```

The next-turn overlay is a separate `VBox` added directly to `mapPanel` (the root StackPane), not inside the `RoutePanel`. This avoids the RoutePanel StackPane being stretched to full window height by the parent layout. The `RoutePanel` handles the collapsible route list at bottom-right; the next-turn overlay floats at top-left, sized to its content.

## Types

### C++ — library layer (`libosmscout-client-java/src/OSMScoutClient.cpp`)

```cpp
struct JavaRouteInstruction {
    double distanceTo;        // meters to next manoeuvre
    std::string turnType;     // "turnLeft", "turnRight", "straightOn", etc.
    std::string streetName;   // street to turn into
    std::string description;  // "Turn left into Hauptstrasse"
    std::string shortDescription; // "Turn left"
};

class JavaRouteInstructionBuilder {
public:
    std::list<JavaRouteInstruction> GenerateRouteInstructions(
        RouteDescription::NodeIterator first,
        RouteDescription::NodeIterator last) const;

    JavaRouteInstruction GenerateNextRouteInstruction(
        RouteDescription::NodeIterator previous,
        RouteDescription::NodeIterator last,
        const GeoCoord &coord) const;
};
```

### Java — library layer (`libosmscout-client-java/java/`)

```java
// New data class in com.framstag.libosmscout.client
public class RouteInstruction {
    public double distanceTo;       // meters
    public TurnType turnType;
    public String streetName;
    public String description;
    public String shortDescription;
}

// New enum in com.framstag.libosmscout.client
public enum TurnType {
    SHARP_LEFT, LEFT, SLIGHTLY_LEFT,
    STRAIGHT_ON,
    SLIGHTLY_RIGHT, RIGHT, SHARP_RIGHT,
    // ... others as needed
}
```

### App layer (`JavaScout/src/`)

The `MainController` wires the `NavigationListener` callbacks to two separate UI elements:
- **`nextTurnBox`** (VBox, direct child of `mapPanel`): Shows the next manoeuvre (icon, distance, description) and an optional "next next" hint row. Positioned at top-left, sized to content.
- **`RoutePanel`** (StackPane, child of `mapPanel`): Collapsible panel at bottom-right showing the full route instruction list via `updateInstructionList()`.

No routing logic lives in the app — only presentation. The `RoutePanel` no longer contains the next-turn display; it only manages the route list and navigation status labels.

## Risks / Trade-offs

- **[Duplication]** `JavaRouteInstructionBuilder` duplicates logic from `RouteDescriptionBuilder` (Qt). → Acceptable: the builder logic is ~50 lines and avoids a Qt dependency in the JNI layer. Can be unified later if a shared non-Qt builder emerges.
- **[JNI performance]** `onNextRouteInstruction` fires on every position update. → Each call creates one small Java object. Negligible cost at 1 Hz GPS rate.
- **[Missing turn types]** If the C++ `DirectionDescription::Move` enum gains new values, the Java `TurnType` enum must be updated. → `onNextRouteInstruction` default method is a no-op; unknown types degrade gracefully (no crash, just no icon).

## "Next Next" Hint

The `JavaRouteInstruction` struct includes optional "next next" fields that carry information about the manoeuvre following the immediate next one. This is populated when the gap between two consecutive instructions is ≤ 200m (e.g. roundabout enter → exit).

**C++ fields:**
- `nextNextDistanceTo` — gap between "next" and "next next" in meters
- `nextNextTurnType` — turn type string
- `nextNextDescription` — human-readable description
- `nextNextShortDescription` — short description

**UI presentation:** The "next next" hint appears as a second row below the main instruction, indented, with smaller grey text. Only shown when the gap is within the threshold.


