package com.framstag.libosmscout.client;

/**
 * Controller for a live navigation session.
 *
 * <p>
 * Created by {@link OSMScoutClient#startNavigation(long, NavigationListener)}
 * after a route has been calculated. Feeds GPS updates to the native navigation
 * engine and forwards engine events to the supplied {@link NavigationListener}.
 */
@SuppressWarnings("restricted")
public class NavigationController {

    /** Native handle referencing the C++ navigation controller. */
    private long nativeHandle;

    /**
     * Protected constructor. Called from JNI via reflection and may be subclassed
     * for testing.
     */
    protected NavigationController() {
    }

    /**
     * Feed a GPS update to the navigation engine.
     *
     * @param lat       latitude in degrees
     * @param lon       longitude in degrees
     * @param speed     speed in m/s, or negative if unknown
     * @param accuracy  horizontal accuracy in meters, or negative if unknown
     * @param timestamp epoch milliseconds of the fix
     */
    public native void processLocation(double lat,
                                         double lon,
                                         double speed,
                                         double accuracy,
                                         long timestamp);

    /**
     * Stop the navigation session and release native resources.
     */
    public native void stop();
}
