package com.framstag.libosmscout.client;

/**
 * Vehicle position state reported by the navigation engine.
 */
public enum NavigationState {
    /** Position has not been initialised yet. */
    Uninitialised,
    /** No GPS signal; last known position is used and may be inaccurate. */
    NoGpsSignal,
    /** Vehicle is on the planned route. */
    OnRoute,
    /** Vehicle has left the planned route; rerouting is requested. */
    OffRoute,
    /** Vehicle position is estimated inside a tunnel. */
    EstimateInTunnel
}
