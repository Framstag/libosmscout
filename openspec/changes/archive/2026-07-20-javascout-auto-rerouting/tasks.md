# Tasks

## 1. MainController reroute handler

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 1.1 Store destination coordinates (`routeDestLat`, `routeDestLon`) when a route is set in `onRouteChanged()` (spec: javascout-auto-rerouting, 1 SP)
- [x] 1.2 Add reroute state fields: `lastRerouteTime`, `rerouting` flag, `REROUTE_COOLDOWN_MS = 15_000` constant (spec: javascout-auto-rerouting, 1 SP)
- [x] 1.3 Implement `onRerouteRequest` handler: check cooldown, set rerouting flag, show "Rerouting..." via route panel, cancel old nav (keep route overlay), call `calculateRouteAsync` from current position to stored destination with same profile (spec: javascout-auto-rerouting, 3 SP)
- [x] 1.4 Handle reroute success in a new `RouteCallback`: update route overlay, start new navigation session, clear rerouting flag, hide "Rerouting..." indicator (spec: javascout-auto-rerouting, 2 SP)
- [x] 1.5 Handle reroute failure: show "Reroute failed" toast for 3 seconds, clear rerouting flag, keep old route and navigation active (spec: javascout-auto-rerouting, 2 SP)
- [x] 1.6 Cancel pending reroute in `stopNavigation()` via `client.cancelRoute()` and reset rerouting flag (spec: javascout-auto-rerouting, 1 SP)

## 2. RoutePanel reroute status

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/RoutePanel.java`*

- [x] 2.1 Add `setRerouteStatus(boolean active, boolean failed)` method that shows/hides a "Rerouting..." label with subtle animation (spec: javascout-auto-rerouting, 2 SP)
- [x] 2.2 Add auto-dismiss timer for "Reroute failed" message (3 seconds) (spec: javascout-auto-rerouting, 1 SP)

## 3. Bug fixes discovered during testing

- [x] 3.1 Add `rerouteGeneration` counter and `isStale()` guard in `NavigationListener` to discard stale JNI callbacks from old controller after reroute (design: Decision 5, 2 SP)
- [x] 3.2 Pause track player during reroute instead of stopping it; add `TrackPlayer.setController()` to swap navigation controller reference after reroute succeeds (design: Decision 6, 2 SP)

## 4. Testing

- [ ] 4.1 Manual smoke test: calculate route, navigate off-route (simulate GPS past a turn), verify reroute fires and new route is displayed (spec: javascout-auto-rerouting, 2 SP)
- [ ] 4.2 Manual test: verify cooldown prevents rapid re-reroutes by feeding multiple off-route GPS fixes within 15 seconds (spec: javascout-auto-rerouting, 1 SP)
- [ ] 4.3 Manual test: verify reroute failure doesn't crash — feed GPS at a location with no routable nodes nearby (spec: javascout-auto-rerouting, 1 SP)
