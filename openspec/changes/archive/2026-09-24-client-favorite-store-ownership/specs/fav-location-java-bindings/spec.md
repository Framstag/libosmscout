# Spec Delta

## REMOVED Requirements

### Requirement: JNI C++ layer delegates to `FavoriteLocationService`
**Reason**: The text described an ownership the native layer no longer has and an initialization that never happened: it required `ClientData` to hold a `FavoriteLocationService` instance, while the code held an owned service pointer that was created on the first load or save and deleted on each replacement - never during `OSMScoutClientBuilder::build()`. The lifetime problem that ownership created is what this change removes: the client now owns a favorite store by value, and the store owns the service instance behind its own lock.

**Migration**: No caller migration. The Java methods, their signatures and their return values are unchanged, and a client still starts without a loaded store. The requirement that follows replaces this one; it is listed under the added requirements as "JNI C++ layer delegates through the client's favorite store".

## ADDED Requirements

### Requirement: JNI C++ layer delegates through the client's favorite store
The native implementation of each Java method SHALL delegate to the C++ `FavoriteLocationService` class through the client's own favorite store. The `ClientData` struct SHALL own that store by value and SHALL NOT own a service instance itself; the store owns the service instance. A client SHALL start without a loaded store, and the first load or save SHALL install one. While no store is loaded, the native side of the favorite methods SHALL report the documented "no store" results instead of faulting. The client's shutdown path SHALL destroy the store through the store's own destruction operation, so that a favorite call running on another thread cannot operate on a destroyed instance.

#### Scenario: Client starts without a loaded store
- **GIVEN** an `OSMScoutClient` built without a favorite file having been loaded
- **WHEN** `getFavoriteGroups` is called
- **THEN** it SHALL return no groups
- **AND** a favorite changing call SHALL report failure instead of faulting

#### Scenario: Loading a file installs a store
- **WHEN** `loadFavoriteLocations` is called with a valid file path
- **THEN** the client SHALL hold a store backed by that file
- **AND** `getFavoriteGroups` SHALL return the groups of that file

#### Scenario: Saving replaces the store as one operation
- **WHEN** `saveFavoriteLocations` is called with a group array
- **THEN** the store SHALL be replaced with that content in one step
- **AND** a concurrent `getFavoriteGroups` SHALL observe either the previous content or the new content, never a partially rebuilt one

#### Scenario: Java methods call through to C++
- **WHEN** any fav location Java method is called
- **THEN** it SHALL invoke the corresponding operation on the client's store, which forwards it to `FavoriteLocationService`

#### Scenario: A call in flight when the client closes
- **GIVEN** a client with a loaded store and a favorite call running on another thread
- **WHEN** the client is closed
- **THEN** the store SHALL be destroyed under its own lock, after the call has left it
- **AND** the closing call SHALL NOT delete an instance that another call is still using
