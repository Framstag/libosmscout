## 1. Data model — extend LocationHighwayMilestoneDescription

- [x] 1.1 Rename existing fields in `LocationHighwayMilestoneDescription` class: `milestoneDistance` → `previousMilestoneDistance`, `milestoneRef` → `previousMilestoneRef`, `milestoneCarriagewayRef` → `previousMilestoneCarriagewayRef`
- [x] 1.2 Add `nextMilestoneDistance` (uint32_t), `nextMilestoneRef` (std::string), `nextMilestoneCarriagewayRef` (std::string) fields, default-initialized to empty
- [x] 1.3 Update all three existing constructors (no-args, with `ObjectFileRef`+`FeatureValueBufferRef`, with distance+bearing+milestone data) to set both previous/next
- [x] 1.4 Add new constructor that accepts separate previous and next milestone data
- [x] 1.5 Add `GetPreviousMilestoneDistance()`, `GetPreviousMilestoneRef()`, `GetPreviousMilestoneCarriagewayRef()` getters (rename from existing)
- [x] 1.6 Add `GetNextMilestoneDistance()`, `GetNextMilestoneRef()`, `GetNextMilestoneCarriagewayRef()` getters
- [x] 1.7 Implement `IsBetweenMilestones()` as computed method: returns true when both previous and next milestones are populated and reference different nodes

## 2. Rewrite DescribeLocationByHighwayMilestone algorithm

- [x] 2.1 Remove BFS connected-way traversal — only use the single closest way from `wayDescription`
- [x] 2.2 Project location onto closest way nodes, find segment containing projection and parametric position `t`
- [x] 2.3 Walk way nodes forward (toward end) from projection segment, detect milestone nodes by coordinate match, stop at first milestone → that is the **next** milestone
- [x] 2.4 Walk way nodes backward (toward start) from projection segment, same detection → that is the **previous** milestone
- [x] 2.5 If both previous and next found and different → create two-milestone description
- [x] 2.6 If only one direction finds a milestone (before-first / after-last) → fallback to single-milestone using that milestone in `previous*` fields
- [x] 2.7 If location exactly at milestone node → both previous and next point to same milestone, `IsBetweenMilestones()` returns false
- [x] 2.8 If no milestone nodes on the way within range → return without milestone description (existing behavior)

## 3. MCP server JSON output

- [x] 3.1 Update `ToJson(const LocationHighwayMilestoneDescription&)` in `MCPServer/src/LocationDescriptionMapper.cpp`: serialize `previousMilestoneDistance`, `previousMilestoneRef`, `previousMilestoneCarriagewayRef`
- [x] 3.2 Add serialization of `nextMilestoneDistance`, `nextMilestoneRef`, `nextMilestoneCarriagewayRef`
- [x] 3.3 Update human-readable text generation: if `IsBetweenMilestones()` → "between milestone X (Ym) and milestone Z (Wm)", else → existing single-milestone text

## 4. Demo and test updates

- [x] 4.1 Update `Demos/src/LocationDescription.cpp` `DumpHighwayMilestoneDescription` to print both previous and next milestone info
- [x] 4.2 Update existing `Tests/src/LocationDescriptionServiceTest.cpp` tests using old `milestoneDistance`/`milestoneRef`/`milestoneCarriagewayRef` → use new `previous*` names
- [x] 4.3 Add test: location between two milestones on same way returns both previous and next
- [x] 4.4 Add test: location before first milestone returns single-milestone fallback
- [x] 4.5 Add test: location after last milestone returns single-milestone fallback
- [x] 4.6 Add test: location exactly at milestone returns same milestone for both, `IsBetweenMilestones()` false
- [x] 4.7 Add test: only one milestone on way returns single-milestone fallback
- [x] 4.8 Add test: `IsBetweenMilestones()` computed correctly (previous == next → false; previous != next → true)

## 5. Build and verify

- [x] 5.1 Build with CMake — verify no compile errors from renamed fields
- [x] 5.2 Run existing tests — verify renamed getters pass
- [x] 5.3 Run new milestone tests — verify all corner cases pass
- [x] 5.4 Verify MCP server builds and milestone JSON output correct