# Spec Delta

## Purpose

Render a map frame into pixel storage that the caller owns, report whether a frame was written, and give
each render destination of the Java client its frame in the pixel layout that destination reads.

## ADDED Requirements

### Requirement: A render writes into caller-owned pixel storage

The Java client SHALL offer a render entry point that writes the frame into storage the caller provides and
SHALL allocate no frame-sized storage of its own for that path. The entry point SHALL report true when a
frame was written and false when the request could not be served. A rejected request SHALL NOT fault and
SHALL NOT report success.

#### Scenario: A served request writes a frame

- **GIVEN** a caller with pixel storage of at least the required size for the viewport
- **WHEN** the buffer render entry point is called with a valid request
- **THEN** it SHALL report true
- **AND** the storage SHALL contain a complete frame for the request

#### Scenario: An unusable request reports failure without faulting

- **GIVEN** a request whose viewport or magnification is rejected, or a client whose database thread is not
  available
- **WHEN** the buffer render entry point is called
- **THEN** it SHALL report false
- **AND** it SHALL NOT fault

#### Scenario: Storage that cannot be written directly is rejected

- **GIVEN** storage that is not directly writable by the native side, or storage smaller than the viewport
  requires
- **WHEN** the buffer render entry point is called
- **THEN** it SHALL report false
- **AND** it SHALL NOT report success for a partially written frame

### Requirement: Each render destination gets the layout it reads

The allocating entry point SHALL hand Java one word per pixel in the layout Java's array-based image
consumers read. The buffer entry point SHALL write four bytes per pixel in the layout a bitmap's own
storage uses. Each destination's frame SHALL be written in its own layout by one place in the code, so a
channel swap between the two layouts SHALL be impossible.

#### Scenario: The array destination reads array words

- **GIVEN** a frame written through the allocating entry point
- **WHEN** the returned pixel data is inspected per pixel
- **THEN** each element SHALL carry the pixel's opacity in its highest byte, then red, then green, then
  blue

#### Scenario: The buffer destination reads bitmap bytes

- **GIVEN** a frame written through the buffer entry point
- **WHEN** the storage is inspected per pixel
- **THEN** the four bytes of a pixel SHALL be red, green, blue and opacity, in that order

#### Scenario: The two destinations agree on the colour of a pixel

- **GIVEN** the same request rendered through both entry points
- **WHEN** a pixel of each frame is read in the layout of its destination
- **THEN** both SHALL report the same red, green and blue values

### Requirement: Both destinations render the same frame

The two render destinations SHALL run the same rendering for the same request and SHALL differ only in where
the frame is written and in the layout their destination states. The buffer entry point SHALL NOT take a
different rendering path, and SHALL NOT retain a reference to the caller's storage after the call.

#### Scenario: One request, one frame, two destinations

- **GIVEN** the same request served once through each entry point
- **WHEN** both frames are compared pixel by pixel in a common layout
- **THEN** every pixel SHALL match

#### Scenario: The caller's storage is only used during the call

- **GIVEN** storage that a caller provides to the buffer render entry point
- **WHEN** the call returns
- **THEN** the bridge SHALL hold no reference to that storage
- **AND** the caller SHALL be able to release or reuse it

### Requirement: An unpainted area is opaque black in every destination

A pixel the style leaves unpainted SHALL be opaque black in both destinations. Each destination's frame
SHALL be cleared in its own layout, because the byte sequence of opaque black differs between them.

#### Scenario: Unpainted area in the buffer destination

- **GIVEN** a request whose style leaves part of the viewport unpainted
- **WHEN** the buffer render entry point is called
- **THEN** those pixels SHALL read as opaque black in the buffer destination's layout

#### Scenario: Unpainted area in the array destination

- **GIVEN** the same request served through the allocating entry point
- **WHEN** the returned pixel data is inspected
- **THEN** those pixels SHALL read as opaque black in the array destination's layout
