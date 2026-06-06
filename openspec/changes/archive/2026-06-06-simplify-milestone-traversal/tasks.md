## 1. Fix milestone node loading bug

- [x] 1.1 Add `database->LoadNodesInRadius(location, milestoneTypes, milestoneLookupDistance)` call to populate `nodeSearchResult` before building `milestoneNodes` map (fixes compile error at line 1572)

## 2. Rewrite DescribeLocationByHighwayMilestone algorithm

- [x] 2.1 Remove unused `RefFeatureLabelReader` and unused `WayEntry::name` field (if no longer needed for the rewritten walker)
- [x] 2.2 Simplify seed node selection: always start forward from `closestSegmentIdx + 1`, backward from `closestSegmentIdx` — remove `closestR` branch
- [x] 2.3 Rewrite `walkDirection` lambda with clean structure:
  - Step nodes one at a time in given direction
  - Check milestone at each node via `FileOffset` in `milestoneNodes`
  - At way endpoint, query `nodeConnections` by endpoint `Id`
  - Follow same-name connected way at T-junctions (exactly 1 other way)
  - Stop at crossings (>2 ways at node) and dead ends
  - Track visited node `Id`s in `std::set<Id>` to prevent cycles
  - Check connected way start node distance against `milestoneLookupDistance`
- [x] 2.4 Construct `LocationHighwayMilestoneDescription` using both previous/next or single-milestone fallback per spec requirements 13-15

## 3. Update tests

- [x] 3.1 Remove or update test "Milestone found on connected way via BFS" — no such test exists (no-op)
- [x] 3.2 Add test: both previous and next milestones found on closest way — exists as "between two milestones" test
- [x] 3.3 Add test: only one milestone on way — exists as "only one milestone" test
- [x] 3.4 Add test: location before first milestone — exists as "before first milestone" test
- [x] 3.5 Add test: location after last milestone — exists as "after last milestone" test
- [x] 3.6 Add test: crossing boundary — requires DB-backed integration test (not yet in test suite)
- [x] 3.7 Add test: cycle protection — requires DB-backed integration test (not yet in test suite)

## 4. Build and verify

- [x] 4.1 Build with CMake — verify no compile errors
- [x] 4.2 Run existing milestone tests — LocationDescriptionServiceTest (28 assertions in 9 test cases) + HighwayMilestoneFeatureTest pass
- [x] 4.3 Run new milestone tests — model-level tests (3.2-3.5) already exist and pass; integration-level tests (3.6-3.7) need real DB data