## 1. Unit table and suffix parsing in Parse()

- [x] 1.1 Add `static constexpr DistanceUnit` struct and `units` array with "km" and "mi" entries in `HighwayMilestoneFeature.cpp` (spec: highway-milestone-distance-units, effort: 1)
- [x] 1.2 Implement suffix detection loop in `Parse()` — strip matched suffix from end, extract numeric prefix, apply conversion factor (spec: highway-milestone-distance-units, effort: 2)
- [x] 1.3 Ensure unit-less values fall through to existing km-default parse path — backward compatible (spec: highway-milestone-feature, effort: 1)
- [x] 1.4 Ensure `"45 + 5"` and unknown suffixes still warn and skip distance (spec: highway-milestone-feature, effort: 1)

## 2. Tests

- [x] 2.1 Add test: `distance=35.0 km` → 35000 m (spec: highway-milestone-distance-units, effort: 1)
- [x] 2.2 Add test: `distance=35 km` → 35000 m (spec: highway-milestone-distance-units, effort: 1)
- [x] 2.3 Add test: `distance=10 mi` → 16093 m (spec: highway-milestone-distance-units, effort: 1)
- [x] 2.4 Add test: `distance=10.5 mi` → 16898 m (spec: highway-milestone-distance-units, effort: 1)
- [x] 2.5 Add test: `distance=35` (no unit) → 35000 m — backward compat (spec: highway-milestone-feature, effort: 1)
- [x] 2.6 Add test: `distance=35.0 nmi` → warn, distance=0 (spec: highway-milestone-feature, effort: 1)
- [x] 2.7 Add test: `distance=35.0 km` with ref → value allocated (spec: highway-milestone-feature, effort: 1)

## 3. Verify build and existing tests pass

- [x] 3.1 Build project with CMake and verify no regressions (effort: 3)
- [x] 3.2 Run existing milestone test suite — all existing scenarios unchanged (effort: 2)
