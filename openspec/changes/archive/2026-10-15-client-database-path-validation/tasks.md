# Tasks

## 1. Registry predicate and validating entry point (spec: database-path-registry)

- [x] 1.1 Add the documented predicate "is this an openable database directory?" to
  `libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h`, declaring it `noexcept` and stating
  that a symbolic link to a directory counts, that every filesystem error is answered as "no", and that it
  is the client's only path check.
- [x] 1.2 Implement the predicate in `libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp` using
  the error-code form of the directory query, so no filesystem error can escape.
- [x] 1.3 Add the validating registration entry point to the header and the source: decide the predicate
  before taking the registry lock, register as before when it holds, and answer false with no set change
  when it does not. Verify: the header documents that a rejected path enters no set and counts no set
  change.
- [x] 1.4 Verify the batch entry point is unchanged and does not call the predicate. Verify: the only call
  site of the predicate outside the tests is the validating entry point and the bridge's report.

## 2. Unit tests (spec: database-path-registry)

- [x] 2.1 Extend `Tests/src/DatabaseOpenTest.cpp` with a self-removing temporary directory, so a case can
  put a real directory and real files in front of the predicate. Verify: the helper removes the tree even
  when a case made it unsearchable.
- [x] 2.2 Cover the predicate: an existing directory (with and without a file in it) is openable, a missing
  path, a regular file and a path below a regular file are not, and a directory that cannot be searched is
  not openable and does not fault. Verify: every scenario of the predicate requirement has a case.
- [x] 2.3 Cover the validating entry point: an existing directory is registered once, a missing path and a
  regular file leave the set untouched, and a rejected path counts no set change. Verify: the case fails if
  the entry point registers before it validates.
- [x] 2.4 Cover that the batch still registers a path that is not a directory. Verify: the case fails if the
  batch is given the predicate.

## 3. Java bridge (spec: client-java-database-paths)

- [x] 3.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, make the single-directory call register
  through the validating entry point, answer failure and return without publishing a set when the path is
  rejected, and report the rejection by directory name only. Verify: no set change is requested for a
  rejected path.
- [x] 3.2 Keep the batch call registering and reporting as before, and add the report for a path of the
  batch that is not an existing directory, by directory name only. Verify: the batch's returned array is
  unchanged for such a path.
- [x] 3.3 Verify the JNI signatures are unchanged, so no Java source file needs an edit. Verify: the diff of
  the capability does not touch `OSMScoutClient.java`.

## 4. Build and regression verification (specs: database-path-registry, client-java-database-paths)

- [x] 4.1 Build the CMake build with the client library, the Java client library and the tests, and verify it
  compiles without errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 4.3 Run `DatabaseOpenTest` through both build systems and verify all cases pass.
- [x] 4.4 Run the rest of the suite and verify no existing test regresses.
- [x] 4.5 Run `openspec validate "client-database-path-validation" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new public API is documented: the predicate's question, its `noexcept` contract, the
  symlink rule and the deliberate batch/single-call asymmetry.
- [x] 5.2 Verify the report lines carry the directory name only and never a full path or a position.
- [x] 5.3 Verify the diff touches only the files listed in the proposal's Impact section, and that the batch
  result for a non-directory path is unchanged.
