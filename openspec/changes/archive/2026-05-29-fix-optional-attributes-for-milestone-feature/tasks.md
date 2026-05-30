## 1. Update Parser

- [x] 1.1 Remove early-return guard for missing distance or ref tags in `Parse()`
- [x] 1.2 Parse `distance` and `ref` independently — present tag sets field, absent tag leaves default
- [x] 1.3 Handle malformed distance: warn, skip distance field (leave 0), still allocate feature value

## 2. Update Tests

- [x] 2.1 Remove test cases expecting no allocation when distance or ref missing
- [x] 2.2 Add test cases for nodes with only `ref`, only `distance`, and no sub-tags
- [x] 2.3 Update malformed-format tests to expect allocated value with distance=0 and other fields preserved
- [x] 2.4 Add round-trip serialization tests for minimal-field scenarios

## 3. Sync Main Spec

- [x] 3.1 Apply delta spec changes to `openspec/specs/highway-milestone-feature/spec.md`
- [x] 3.2 Verify spec reflects new optional-attribute behavior