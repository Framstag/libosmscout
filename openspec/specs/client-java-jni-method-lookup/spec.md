# client-java-jni-method-lookup Specification

## Purpose
Define how the Java client's JNI bridge resolves the classes and methods it needs, so that a mismatch between the bridge and the Java classes it talks to degrades into a refused dispatch instead of aborting the process.

## Requirements

### Requirement: A failed JNI lookup leaves no exception pending

Every class, instance-method and static-method lookup the navigation bridge performs SHALL clear the exception a failed lookup leaves pending, so that a class the bridge cannot find and a method descriptor that matches nothing both yield a null id. With no exception left pending, the next JNI call SHALL proceed and the null guard at the call site SHALL be what the caller observes.

#### Scenario: A mismatching instruction-constructor descriptor yields no instructions

- **GIVEN** the bridge's cached `RouteInstruction` constructor descriptor matches no constructor of the class
- **WHEN** the bridge resolves its navigation-listener method ids
- **THEN** the constructor lookup SHALL yield a null id and SHALL leave no exception pending
- **AND** the instruction dispatch SHALL deliver no instruction list
- **AND** the process SHALL NOT abort

#### Scenario: A missing class yields a null id without aborting the bridge

- **WHEN** one of the classes the bridge looks up does not exist in the application
- **THEN** that lookup SHALL yield a null id and SHALL leave no exception pending
- **AND** the remaining lookups SHALL still complete

#### Scenario: A complete set of ids dispatches as before

- **WHEN** every class and every method the bridge looks up exists with the expected descriptor
- **THEN** the listener's methods SHALL be called with the same arguments as before

### Requirement: The instruction array is built before it is allocated

The bridge SHALL build every Java route instruction before it allocates the array it delivers, so that an instruction it cannot build is not a null entry in an array sized for the input. When it could build none of them, it SHALL dispatch nothing.

#### Scenario: No buildable instruction means no dispatch

- **WHEN** the bridge can build none of the instructions of a message
- **THEN** it SHALL NOT call the listener's instruction callback
- **AND** it SHALL NOT pass an array

#### Scenario: A partially buildable list has no null entries

- **WHEN** the bridge can build some of the instructions of a message
- **THEN** the delivered array SHALL hold exactly those, in their original order
- **AND** it SHALL contain no null entry

#### Scenario: A complete list keeps its length and order

- **WHEN** the bridge can build every instruction of a message
- **THEN** the delivered array's length and order SHALL be the input's
