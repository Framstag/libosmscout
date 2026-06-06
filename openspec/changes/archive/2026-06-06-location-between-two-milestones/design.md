## Context

`DescribeLocationByHighwayMilestone` currently finds the single closest highway milestone node to a location, using BFS over connected ways within `milestoneLookupDistance`. Result stored as `LocationHighwayMilestoneDescription` with one `milestoneDistance`/`milestoneRef`/`milestoneCarriagewayRef` set.

Change: describe location as between two milestones (backward + forward along way) instead of relative to nearest only.

## Goals / Non-Goals

**Goals:**
- Return up to two milestones bracketing location along same way
- Corner-case handling: before-first, after-last, only-one, same-coord
- Full backward compat: MCP JSON keeps old fields when only one milestone
- MCP JSON adds `previous*` / `next*` prefixed fields for two-milestone case

**Non-Goals:**
- No change to other description types (crossing, at-place, way, etc.)
- No new DB schema or import pipeline changes
- No rework of `LocationDescription` class itself — just the milestone description subclass

## Decisions

### D1: Extend existing class — rename and add second milestone fields

Existing `milestoneDistance`/`milestoneRef`/`milestoneCarriagewayRef` renamed with `previous` prefix: `previousMilestoneDistance`, `previousMilestoneRef`, `previousMilestoneCarriagewayRef`.

New `next` prefixed fields for second milestone: `nextMilestoneDistance`, `nextMilestoneRef`, `nextMilestoneCarriagewayRef`.

`IsBetweenMilestones()` is a **computed** method returning true when both `previous` and `next` milestones are set and reference different milestone nodes.

**Alternative considered**: Stored `isBetweenMilestones` flag. Rejected — state derivable from field presence, avoids stale flag bugs.

### D2: Milestone ordering by way progression, not by distance from location

Current code picks closest milestone by straight-line distance. Two-milestone needs ordering along way:
1. Project location onto nearest way segment → get position `t` along way
2. Walk way nodes forward/backward from projection to find milestones
3. Sort milestones by way node index × parametric position, not by Euclidean distance

**Alternative considered**: Sort by milestone distance value (e.g., "35" vs "50"). Rejected because milestone distance can be non-monotonic or missing. Way topology is the reliable ordering.

### D3: Side-of-milestone determination via way node traversal

To determine if location is "before" or "after" a milestone along a way:
1. Insert projection point into way node sequence (virtual node at parametric position)
2. Find the milestone position in same node sequence  
3. If projection comes before milestone in sequence → location is before that milestone
4. If projection comes after → location is after

This naturally handles bidirectional ways (both directions share same node sequence).

### D4: Same way only — no cross-way search

Both milestones MUST be on the same way. If only one milestone found on the closest way (or location is before first / after last), fall back to single-milestone description using the nearest milestone.

Existing BFS for connected ways **removed** — algorithm only walks nodes of the single closest way from `DescribeLocationByWay`.

## Algorithm Outline

```
1. Get way description from `DescribeLocationByWay`
2. Load the single closest way (same way used for way description)
3. Collect all milestone nodes on that way within `milestoneLookupDistance`
4. Project location onto way → get projected coord + parametric position `t`
5. Find the segment of the way containing the projection
6. Walk way nodes **forward** (toward end of way) from the containing segment:
   - At each node, check if a milestone exists (coordinate match)
   - Stop at first milestone found → this is the **next** milestone
7. Walk way nodes **backward** (toward start of way) from the containing segment:
   - At each node, check if a milestone exists
   - Stop at first milestone found → this is the **previous** milestone
8. If both previous and next milestones found AND they are different:
   → Create two-milestone description with `previous*` + `next*` fields
9. If only one direction found (previous == next, or only one exists):
   → Fallback to single-milestone using that milestone's data in `previous*` fields
```

### Corner Case Handling

| Case | Behavior |
|------|----------|
| Location before first milestone | Next milestone found, previous none → single milestone fallback |
| Location after last milestone | Previous milestone found, next none → single milestone fallback |
| Only one milestone on way | Single milestone fallback using `previous*` fields |
| Location exactly at milestone | Previous == next same milestone → "at milestone X" |
| No milestones within range | Return without milestone description (existing) |
| Location at start/end of way | Walk from that endpoint; only one direction traversable |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Performance: walking way nodes per `DescribeLocation` call | Limit milestone search distance (existing `milestoneLookupDistance`); milestone nodes are sparse |
| Non-monotonic milestone distances along way | Order by way topology (node index × t), not by milestone distance value |
| Milestone ref + carriageway differ between previous/next | Each stores own ref + carriagewayRef; display shows e.g. "between A2 (35m) and A3 (50m)" |
| MCP JSON backward compat | Old flat fields renamed to `previous*`; new `next*` fields added |

## Open Questions


