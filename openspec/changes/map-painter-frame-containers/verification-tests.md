# Verification: unit tests (tasks 3.1-3.6)

Test file: `Tests/src/MapPainterFrameBuffersTest.cpp` (5 test cases, 125 assertions,
all passing). Registered in `Tests/CMakeLists.txt` and `Tests/meson.build`, linking only
`osmscout` + `osmscoutmap` + Catch2 (no graphics backend needed).

```
cd build && ninja Tests/MapPainterFrameBuffersTest
./Tests/MapPainterFrameBuffersTest
-> All tests passed (125 assertions in 5 test cases)
```

The tests drive the painter directly with synthetic objects through
`osmscout::MapPainterNoOp`, whose subclass records what the backend sees: the prepared
data in the post-preprocessing callback and the order of the `DrawArea`/`DrawWay` calls.

| requirement | test case | result |
|-------------|-----------|--------|
| Prepared frame data is reused across frames | `Prepared stores are reused between frames` | pass |
| Prepared frame data is contiguous per object kind | `Prepared areas and ways are contiguous` | pass |
| Draw order is stable for prepared areas and ways | `Equal-comparing areas keep their preparation order` | pass |
| Route labels resolve their prepared way path | `Route labels keep resolving their prepared way path` | pass |
| Backends retain read access to prepared areas and ways | `Backend callback sees every prepared area and way in draw order` | pass |

## Sensitivity of the tests (temporary mutations, reverted afterwards)

| mutation | effect |
|----------|--------|
| `std::sort` instead of `std::stable_sort` for the prepared areas | ordering test fails: the 32 equal-comparing areas are no longer in preparation order |
| route label index `wayPathData.size()` instead of `size()-1` | route label test aborts on the out-of-range access (SIGSEGV); with the correct index the label path geometry matches the route segment within 2 px |

The `git diff` contains no trace of these mutations; verified after reverting
(`grep TEMP-PROBE` empty, `wayPathData.size()-1` present).

## Coverage limits found while writing the tests

- `Way::fileOffset` and `Area::fileOffset` have no public setter and no friend for tests,
  so every synthetic object carries offset 0. Two consequences:
  - The route label test can only use a single route member way. Growing the prepared way
    path store with several *distinct* member ways is not expressible in a unit test.
    The append case that occurs in production - a route member way that is not part of the
    rendered data and is resolved through `Route::SetResolvedMembers` - is covered, and
    further prepared area data is added in that frame so the ordered stores are rebuilt.
  - The spec scenario "Route label is placed on its own route segment" is asserted through
    the registered label (ref, text) plus the label path geometry projected to the route
    segment, because the offset alone is degenerate in synthetic data.
- `GetAreaData()` / `GetWayData()` turned out to be **protected**, not public (a backend
  reads them from its subclass). The test painter exposes them through two public
  pass-through methods; the proposal and design were corrected accordingly.
