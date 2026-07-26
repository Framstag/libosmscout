package com.framstag.libosmscout.client;

/**
 * Vehicle type for routing and navigation.
 * <p>
 * Maps to C++ {@code osmscout::Vehicle} enum values.
 */
public enum Vehicle {
    /** Car / motor vehicle routing. */
    CAR,
    /** Bicycle routing (uses cycleways, paths, residential streets). */
    BICYCLE,
    /** Pedestrian / foot routing (uses footways, paths, pedestrian zones). */
    PEDESTRIAN
}
