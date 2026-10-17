# Spec Delta

## Purpose

JNI bridge to libosmscout routing engine for async route calculation between two coordinates.

## MODIFIED Requirements

### Requirement: RouteCallback Java interface

The Java interface `RouteCallback` SHALL carry the failure kinds as integer constants — `KIND_UNKNOWN`,
`KIND_ROUTING_DATA_UNAVAILABLE`, `KIND_START_UNROUTABLE`, `KIND_TARGET_UNROUTABLE`, `KIND_NO_ROUTE_FOUND`,
`KIND_ROUTING_DATA_FAILURE`, `KIND_ENGINE_FAILURE` — and the methods `onProgress(int percent)`,
`onSuccess(RouteEntry route)`, `onError(String message)`, `onErrorDetail(int kind, String detail, double
nearestRoadDistanceMeters)` and `onCancel()`. `onErrorDetail` SHALL be a default method that delegates to
`onError(detail)`, and the kind numbering SHALL be identical on the Java and the native side, because the
integer is what crosses the boundary. It SHALL be passed to the route calculation and invoked from JNI.

#### Scenario: Callback methods invoked from JNI

- **WHEN** route calculation progresses, completes, fails, or is cancelled
- **THEN** the corresponding callback method is called on the Java thread

#### Scenario: A failure reports a kind a caller can act on

- **GIVEN** a calculation that fails because an end cannot be attached to the road network
- **WHEN** the failure reaches Java
- **THEN** `onErrorDetail` SHALL be called with `KIND_START_UNROUTABLE` or `KIND_TARGET_UNROUTABLE`
- **AND** the detail SHALL be an English description for a log, not display text

#### Scenario: An implementation written before the method existed keeps working

- **GIVEN** a `RouteCallback` implementation that overrides only `onError(String)`
- **WHEN** a calculation fails with a kind
- **THEN** the default `onErrorDetail` SHALL call `onError` with the detail
- **AND** no error SHALL be raised for the missing override

#### Scenario: An unknown kind is the unknown kind

- **GIVEN** a failure whose kind this interface does not define
- **WHEN** the kind reaches Java
- **THEN** it SHALL be reported as one of the interface's kinds or as `KIND_UNKNOWN`
- **AND** a caller that does not know it SHALL be able to treat it as `KIND_UNKNOWN`
