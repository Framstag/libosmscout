## 1. Revert `DBThread::Initialize()`

- [x] 1.1 Restore `mapManager->LookupDatabases();` call in `DBThread::Initialize()`, remove the deferral comment
- [x] 1.2 Verify Qt client (`OSMScout2`) compiles and starts with automatic database discovery

## 2. Remove duplicate scan from JavaScout

- [x] 2.1 Delete the explicit `LookupDatabases()` call and `.wait()` from `OSMScoutClient.cpp` build method
- [x] 2.2 Verify JavaScout compiles and databases are still discovered at startup

## 3. Verify

- [x] 3.1 Build both Qt and JavaScout targets with no errors
- [x] 3.2 Run existing C++ test suite — all tests pass
- [x] 3.3 Run JavaScout test suite — all tests pass
