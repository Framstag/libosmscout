package com.framstag.libosmscout.client;

import java.util.List;

/**
 * Client object for libosmscout.
 *
 * An instance is created via {@link OSMScoutClientBuilder}.
 * Call {@link #close()} to release native resources when done.
 */
@SuppressWarnings("restricted")
public class OSMScoutClient {

    static {
        System.loadLibrary("osmscout_client_java");
    }

    /** Native handle referencing the C++ ClientData object. */
    private long nativeHandle;

    /** Package-private constructor, called from JNI. */
    OSMScoutClient() {
    }

    /**
     * Open a map database directory.
     *
     * @param path absolute filesystem path to a directory containing .osmscout map data
     * @return true if the path was registered successfully, false on failure
     */
    public native boolean openDatabase(String path);

    /**
     * Close the database and release all native C++ resources.
     *
     * @return true if resources were released, false if not initialised
     */
    public native boolean close();

    /**
     * Check if the client is fully initialised with native resources.
     *
     * @return true if initialised, false otherwise
     */
    public native boolean isInitialized();

    /**
     * Render the current map view to an ARGB pixel array.
     * <p>
     * Uses the Cairo backend to render the map at the given position and zoom level.
     * The returned array has length {@code width * height}, each int is ARGB format
     * (0xAARRGGBB).
     *
     * @param width        viewport width in pixels
     * @param height       viewport height in pixels
     * @param lat          center latitude in degrees
     * @param lon          center longitude in degrees
     * @param angle        map rotation angle in radians (0 = north-up)
     * @param magnification magnification level (0 = world, higher = more zoomed in)
     * @return int[] ARGB pixel data, or null if not initialised or invalid params
     */
    public native int[] render(int width, int height,
                               double lat, double lon,
                               double angle,
                               int magnification);

    /**
     * Sentinel for "no default admin region" — pass to
     * {@link #searchLocations(String, int, long)} for an unconstrained search.
     */
    public static final long NO_ADMIN_REGION = 0L;

    /**
     * Search for locations matching a free-text query.
     * <p>
     * Uses the core {@code LocationService::SearchForLocationByString()} to find
     * admin regions, locations, POIs, and addresses matching the query, and the
     * text search index for free-text hits on named objects.
     * Results are sorted by relevance (type priority, distance, match quality).
     *
     * @param query free-text search string (e.g. "Berlin", "Dortmund Hbf")
     * @param limit maximum number of results to return
     * @param adminRegionHandle handle of a resolved admin region (see
     *        {@link #resolveAdminRegion(double, double)}) used as default region
     *        fallback for incomplete queries, or {@link #NO_ADMIN_REGION} for an
     *        unconstrained search
     * @return array of matching LocationEntry objects, or empty array if none found
     */
    public native LocationEntry[] searchLocations(String query, int limit, long adminRegionHandle);

    /**
     * Search for locations matching a free-text query with a region-name
     * default region and optional cancellation.
     * <p>
     * Uses the core {@code LocationService::SearchForLocationByString()} to find
     * admin regions, locations, POIs, and addresses matching the query, and the
     * text search index for free-text hits on named objects. Region scoping and
     * cancellation mirror OSMScout2 behaviour.
     *
     * @param query         free-text search string (e.g. "Berlin", "Dortmund Hbf")
     * @param limit         maximum number of results to return
     * @param defaultRegion optional region name to scope the search, or null
     * @param cancel        if true, cancel any in-progress search and return empty
     * @return array of matching LocationEntry objects, or empty array if none found
     */
    public native LocationEntry[] searchLocations(String query, int limit, String defaultRegion, boolean cancel);

    /**
     * Search for locations matching a free-text query without region context.
     * <p>
     * Convenience overload of
     * {@link #searchLocations(String, int, String, boolean)}.
     *
     * @param query free-text search string
     * @param limit maximum number of results to return
     * @return array of matching LocationEntry objects, or empty array if none found
     */
    public LocationEntry[] searchLocations(String query, int limit) {
        return searchLocations(query, limit, null, false);
    }

    /**
     * Cancel the currently running location search, if any.
     * <p>
     * A new search cancels the previously running one automatically; this
     * method allows explicit cancellation (e.g. from a cancel button).
     */
    public native void cancelSearch();

    /**
     * Search for POIs of the given OSM types within a radius around a coordinate.
     * <p>
     * Resolves the type names against the loaded databases' type configs and
     * calls the native POI service ({@code POIService::GetPOIsInRadius}).
     * Results are sorted by distance from the search center (nearest first).
     *
     * @param typeNames    OSM type names (e.g. "tourism_hotel", "shop")
     * @param lat          center latitude in degrees
     * @param lon          center longitude in degrees
     * @param radiusMeters search radius in meters
     * @param limit        maximum number of results to return
     * @return array of matching PoiEntry objects, or empty array if none found
     */
    public native PoiEntry[] searchPOIsByTypes(String[] typeNames,
                                               double lat,
                                               double lon,
                                               double radiusMeters,
                                               int limit);

    /**
     * Search for POIs of the given category within a radius around a coordinate.
     * <p>
     * Resolves the category to its hardcoded OSM type names via
     * {@link PoiCategories} and delegates to
     * {@link #searchPOIsByTypes(String[], double, double, double, int)}.
     *
     * @param category     category id (see {@link PoiCategories#HOTELS},
     *                     {@link PoiCategories#RESTAURANTS}, {@link PoiCategories#GROCERY})
     * @param lat          center latitude in degrees
     * @param lon          center longitude in degrees
     * @param radiusMeters search radius in meters
     * @param limit        maximum number of results to return
     * @return array of matching PoiEntry objects, or empty array if none found
     */
    public PoiEntry[] searchPOIs(String category,
                                 double lat,
                                 double lon,
                                 double radiusMeters,
                                 int limit) {
        String[] typeNames = PoiCategories.getTypeNames(category);
        if (typeNames == null || typeNames.length == 0 || radiusMeters <= 0) {
            return new PoiEntry[0];
        }
        return searchPOIsByTypes(typeNames, lat, lon, radiusMeters, limit);
    }

    /**
     * Get the name of the admin region containing the given coordinate.
     * <p>
     * Reverse lookup via {@code LocationDescriptionService::ReverseLookupRegion}.
     * Used to scope location searches to the current map region.
     *
     * @param lat latitude in degrees
     * @param lon longitude in degrees
     * @return admin region name, or null if no region found
     */
    public native String getRegion(double lat, double lon);

    /**
     * Resolve the admin region containing the given coordinate.
     * <p>
     * Walks the location index region hierarchy and returns an opaque handle to
     * the deepest admin region whose boundary contains the coordinate, or 0 if
     * no region is found (or the database is not initialised). The returned
     * handle SHALL be released with {@link #releaseAdminRegion(long)} when no
     * longer needed.
     *
     * @param lat latitude in degrees
     * @param lon longitude in degrees
     * @return opaque admin region handle, or 0 if none found
     */
    public native long resolveAdminRegion(double lat, double lon);

    /**
     * Release a previously resolved admin region handle.
     *
     * @param handle handle returned by {@link #resolveAdminRegion(double, double)}
     */
    public native void releaseAdminRegion(long handle);

    /**
     * Get the name of a previously resolved admin region.
     *
     * @param handle handle returned by {@link #resolveAdminRegion(double, double)}
     * @return region name, or null if the handle is unknown
     */
    public native String getAdminRegionName(long handle);

    /**
     * Get a structured description of the most reasonable visible object
     * at the given geographic coordinate.
     * <p>
     * Queries objects in a small bounding box around the coordinate,
     * ranks them by (has description data, visible at zoom, proximity),
     * and returns a structured {@link ObjectDescription} for the best match.
     *
     * @param lat latitude in degrees
     * @param lon longitude in degrees
     * @param magnification current map magnification (zoom level)
     * @return ObjectDescription with entries, or empty description if no object found
     */
    public native ObjectDescription getDescription(double lat, double lon, int magnification);

    /**
     * Get the bounding box of the most reasonable visible object
     * at the given geographic coordinate.
     * <p>
     * Queries objects in a small bounding box around the coordinate,
     * ranks them by (has description data, visible at zoom, proximity),
     * and returns the bounding box of the best match.
     *
     * @param lat latitude in degrees
     * @param lon longitude in degrees
     * @param magnification current map magnification (zoom level)
     * @return double[]{minLat, maxLat, minLon, maxLon} for area/way objects,
     *         or null if the best match is a node or no object found
     */
    public native double[] getObjectBoundingBox(double lat, double lon, int magnification);

    /**
     * Get a list of structured descriptions of all reasonable visible objects
     * at the given geographic coordinate.
     * <p>
     * Queries objects in a small bounding box around the coordinate, ranks
     * them by (has description data, visible at the given magnification,
     * proximity), and returns one {@link ObjectDescription} per ranked
     * candidate, each carrying its object identity (ref type, type name,
     * file offset).
     *
     * @param lat          latitude in degrees
     * @param lon          longitude in degrees
     * @param magnification current map magnification (0 = world, higher = more zoomed in)
     * @return ranked list of candidate descriptions, or empty list if no object found
     */
    public native List<ObjectDescription> getDescriptionCandidates(double lat, double lon, int magnification);

    /**
     * Calculate a route between two coordinates asynchronously with a routing profile.
     * <p>
     * Like {@link #calculateRouteAsync(double, double, double, double, RouteCallback)}
     * but accepts a {@link RoutingProfile} to specify vehicle type and avoid flags.
     *
     * @param startLat  start latitude in degrees
     * @param startLon  start longitude in degrees
     * @param destLat   destination latitude in degrees
     * @param destLon   destination longitude in degrees
     * @param profile   routing profile (vehicle type, avoid flags)
     * @param callback  callback for progress, success, error, and cancel events
     */
    public void calculateRouteAsync(double startLat, double startLon,
                                   double destLat, double destLon,
                                   RoutingProfile profile,
                                   RouteCallback callback) {
        calculateRouteWithObjectsAsync(startLat, startLon, 0, null,
                                       destLat, destLon, 0, null,
                                       profile, callback);
    }

    /**
     * Calculate a route between two coordinates asynchronously.
     * <p>
     * Starts route calculation on a background thread. Progress, success,
     * error, and cancel events are reported via the {@link RouteCallback}.
     * The callback methods are invoked from the native thread — marshal to
     * the UI thread if needed.
     * <p>
     * Only one route calculation may be active at a time. Call
     * {@link #cancelRoute()} to abort an in-progress calculation.
     *
     * @param startLat  start latitude in degrees
     * @param startLon  start longitude in degrees
     * @param destLat   destination latitude in degrees
     * @param destLon   destination longitude in degrees
     * @param callback  callback for progress, success, error, and cancel events
     */
    public native void calculateRouteAsync(double startLat, double startLon,
                                           double destLat, double destLon,
                                           RouteCallback callback);

    /**
     * Calculate a route between two locations with object references and a routing profile.
     * <p>
     * Like {@link #calculateRouteWithObjectsAsync(double, double, long, String, double, double, long, String, RouteCallback)}
     * but accepts a {@link RoutingProfile} to specify vehicle type and avoid flags.
     *
     * @param startLat        start latitude
     * @param startLon        start longitude
     * @param startObjOffset  object file offset from LocationEntry, or 0
     * @param startObjType    object type string ("node"/"way"/"area") or null
     * @param destLat         destination latitude
     * @param destLon         destination longitude
     * @param destObjOffset   object file offset from LocationEntry, or 0
     * @param destObjType     object type string or null
     * @param profile         routing profile (vehicle type, avoid flags)
     * @param callback        callback for progress, success, error, cancel
     */
    public void calculateRouteWithObjectsAsync(double startLat, double startLon,
                                                long startObjOffset, String startObjType,
                                                double destLat, double destLon,
                                                long destObjOffset, String destObjType,
                                                RoutingProfile profile,
                                                RouteCallback callback) {
        calculateRouteWithObjectsWithProfile(startLat, startLon,
            startObjOffset, startObjType,
            destLat, destLon,
            destObjOffset, destObjType,
            profile, callback);
    }

    /**
     * Native implementation for route calculation with object references and profile.
     *
     * @param startLat        start latitude
     * @param startLon        start longitude
     * @param startObjOffset  object file offset from LocationEntry, or 0
     * @param startObjType    object type string ("node"/"way"/"area") or null
     * @param destLat         destination latitude
     * @param destLon         destination longitude
     * @param destObjOffset   object file offset from LocationEntry, or 0
     * @param destObjType     object type string or null
     * @param profile         routing profile (vehicle type, avoid flags)
     * @param callback        callback for progress, success, error, cancel
     */
    native void calculateRouteWithObjectsWithProfile(double startLat, double startLon,
                                                      long startObjOffset, String startObjType,
                                                      double destLat, double destLon,
                                                      long destObjOffset, String destObjType,
                                                      RoutingProfile profile,
                                                      RouteCallback callback);

    /**
     * Calculate a route between two locations with object references.
     * <p>
     * Like {@link #calculateRouteAsync(double, double, double, double, RouteCallback)}
     * but also accepts object file offsets from location search results.
     * The routing engine first tries to find routable nodes via the object
     * references (e.g. the road a building is on), falling back to coordinate
     * search if that fails.
     *
     * @param startLat        start latitude
     * @param startLon        start longitude
     * @param startObjOffset  object file offset from LocationEntry, or 0
     * @param startObjType    object type string ("node"/"way"/"area") or null
     * @param destLat         destination latitude
     * @param destLon         destination longitude
     * @param destObjOffset   object file offset from LocationEntry, or 0
     * @param destObjType     object type string or null
     * @param callback        callback for progress, success, error, cancel
     */
    public native void calculateRouteWithObjectsAsync(double startLat, double startLon,
                                                       long startObjOffset, String startObjType,
                                                       double destLat, double destLon,
                                                       long destObjOffset, String destObjType,
                                                       RouteCallback callback);

    /**
     * Cancel an in-progress route calculation.
     * <p>
     * Sets a {@code Breaker} flag checked by the routing algorithm.
     * The {@link RouteCallback#onCancel()} method will be invoked
     * when cancellation completes.
     */
    public native void cancelRoute();

    /**
     * Start a live navigation session for the route identified by
     * {@link RouteEntry#routeHandle} with the given vehicle type.
     * <p>
     * Delegates to {@link #startNavigationWithVehicle(long, Vehicle, NavigationListener)}.
     *
     * @param routeHandle opaque handle from the last successful route calculation
     * @param vehicle     vehicle type for navigation (affects DataAgent filtering)
     * @param listener    callback receiver for navigation events
     * @return a {@link NavigationController} on success, or null if the handle is invalid
     */
    public NavigationController startNavigation(long routeHandle,
                                                Vehicle vehicle,
                                                NavigationListener listener) {
        return startNavigationWithVehicle(routeHandle, vehicle, listener);
    }

    /**
     * Native implementation for startNavigation with vehicle type.
     *
     * @param routeHandle opaque handle from the last successful route calculation
     * @param vehicle     vehicle type for navigation (affects DataAgent filtering)
     * @param listener    callback receiver for navigation events
     * @return a {@link NavigationController} on success, or null if the handle is invalid
     */
    native NavigationController startNavigationWithVehicle(long routeHandle,
                                                           Vehicle vehicle,
                                                           NavigationListener listener);

    /**
     * Start a live navigation session for the route identified by
     * {@link RouteEntry#routeHandle} with default car vehicle.
     *
     * @param routeHandle opaque handle from the last successful route calculation
     * @param listener    callback receiver for navigation events
     * @return a {@link NavigationController} on success, or null if the handle is invalid
     */
    public native NavigationController startNavigation(long routeHandle,
                                                       NavigationListener listener);

    /**
     * Import the first track from a GPX file.
     * <p>
     * Uses the libosmscout-gpx library to parse the file and returns the points
     * of the first track (all segments concatenated). Returns an empty array if the
     * file cannot be read, contains no tracks, or GPX support is not compiled in.
     *
     * @param filePath absolute filesystem path to a GPX file
     * @return array of {@link TrackPoint} objects, or empty array on error
     */
    public native TrackPoint[] importGpxTrack(String filePath);

    /**
     * Render the current map view to an ARGB pixel array, with optional route,
     * track, and POI marker overlays.
     * <p>
     * Same as {@link #render(int, int, double, double, double, int)} but also draws a route
     * polyline, an imported track polyline, start/end markers, favorite markers, and
     * a selected-search marker on the map. The route, track, and favorite waypoints are
     * passed as parallel arrays of latitudes and longitudes. The selected search
     * coordinate uses {@code Double.NaN} for latitude to mean "no selection".
     *
     * @param width        viewport width in pixels
     * @param height       viewport height in pixels
     * @param lat          center latitude in degrees
     * @param lon          center longitude in degrees
     * @param angle        map rotation angle in radians (0 = north-up)
     * @param magnification magnification level (0 = world, higher = more zoomed in)
     * @param routeLats    array of route waypoint latitudes, or null for no route
     * @param routeLons    array of route waypoint longitudes, or null for no route
     * @param favoriteLats array of favorite latitudes, or null for no favorites
     * @param favoriteLons array of favorite longitudes, or null for no favorites
     * @param searchSelLat latitude of the selected search result, or {@code Double.NaN}
     * @param searchSelLon longitude of the selected search result, ignored when no selection
     * @param trackLats        array of imported track latitudes, or null for no track
     * @param trackLons        array of imported track longitudes, or null for no track
     * @return int[] ARGB pixel data, or null if not initialised or invalid params
     */
    public native int[] renderWithRouteAndPois(int width, int height,
                                               double lat, double lon,
                                               double angle,
                                               int magnification,
                                               double[] routeLats,
                                               double[] routeLons,
                                               double[] favoriteLats,
                                               double[] favoriteLons,
                                               double searchSelLat,
                                               double searchSelLon,
                                               double[] trackLats,
                                               double[] trackLons);

    /**
     * Render the current map view to an ARGB pixel array, with optional route overlay.
     * <p>
     * Convenience overload that calls {@link #renderWithRouteAndPois(int, int, double,
     * double, double, int, double[], double[], double[], double[], double, double, double[], double[])}
     * with no track, favorite, or selected-search markers.
     *
     * @param width        viewport width in pixels
     * @param height       viewport height in pixels
     * @param lat          center latitude in degrees
     * @param lon          center longitude in degrees
     * @param angle        map rotation angle in radians (0 = north-up)
     * @param magnification magnification level (0 = world, higher = more zoomed in)
     * @param routeLats    array of route waypoint latitudes, or null for no route
     * @param routeLons    array of route waypoint longitudes, or null for no route
     * @return int[] ARGB pixel data, or null if not initialised or invalid params
     */
    public int[] renderWithRoute(int width, int height,
                                 double lat, double lon,
                                 double angle,
                                 int magnification,
                                 double[] routeLats,
                                 double[] routeLons) {
        return renderWithRouteAndPois(width, height, lat, lon, angle, magnification,
                                      routeLats, routeLons,
                                      null, null,
                                      Double.NaN, Double.NaN,
                                      null, null);
    }

    /**
     * Set or hide the GPS location marker that is drawn on top of the map during
     * the next render. The marker is rendered in the same native pass as the map,
     * so it always uses the exact same projection and cannot drift relative to the
     * road. Call with {@code Double.NaN} for latitude to hide the marker.
     *
     * @param lat     marker latitude in degrees, or NaN to hide
     * @param lon     marker longitude in degrees
     * @param bearing marker bearing in degrees, 0 = north, clockwise, or -1 if unknown
     * @param accuracy horizontal accuracy in meters, or -1/NaN if unknown
     */
    public native void setGpsMarker(double lat, double lon, double bearing, double accuracy);

    /**
     * Project a geographic coordinate to screen pixels for the given map view.
     *
     * @param width      viewport width in pixels
     * @param height     viewport height in pixels
     * @param centerLat  map center latitude in degrees
     * @param centerLon  map center longitude in degrees
     * @param magnification magnification level
     * @param dpi        physical dots-per-inch of the display
     * @param angle      map rotation angle in radians (0 = north-up)
     * @param lat        latitude to project
     * @param lon        longitude to project
     * @return double[]{x, y} in viewport pixel coordinates, or null if invalid/outside
     */
    public native double[] projectToPixel(int width, int height,
                                          double centerLat, double centerLon,
                                          int magnification, double dpi,
                                          double angle,
                                          double lat, double lon);

    // ---- Favorite Locations ----

    /**
     * Load favorite locations from a JSON file.
     *
     * @param filePath absolute path to the favorites JSON file
     * @return true if loaded successfully, false on parse error
     */
    public native boolean loadFavoriteLocations(String filePath);

    /**
     * Save favorite location groups to a JSON file.
     *
     * @param filePath absolute path to the favorites JSON file
     * @param groups   array of groups to persist
     * @return true if saved successfully, false on write error
     */
    public native boolean saveFavoriteLocations(String filePath, FavoriteLocationGroup[] groups);

    /**
     * Return all loaded favorite location groups.
     *
     * @return array of groups, or empty array if none loaded
     */
    public native FavoriteLocationGroup[] getFavoriteGroups();

    /**
     * Add a new empty group.
     *
     * @param name group name (must be unique)
     * @return true if added, false if name already exists
     */
    public native boolean addGroup(String name);

    /**
     * Delete a group and all its favorites.
     *
     * @param name group name
     * @return true if deleted, false if not found
     */
    public native boolean deleteGroup(String name);

    /**
     * Rename a group.
     *
     * @param oldName current group name
     * @param newName new group name (must be unique)
     * @return true if renamed, false if oldName not found or newName already exists
     */
    public native boolean renameGroup(String oldName, String newName);

    /**
     * Add a favorite to a group.
     *
     * @param groupName group name
     * @param favName   favorite name (must be unique within group)
     * @param lat       latitude in degrees
     * @param lon       longitude in degrees
     * @return true if added, false if group not found or duplicate name
     */
    public native boolean addFavorite(String groupName, String favName, double lat, double lon);

    /**
     * Delete a favorite from a group.
     *
     * @param groupName group name
     * @param favName   favorite name to delete
     * @return true if deleted, false if group or fav not found
     */
    public native boolean deleteFavorite(String groupName, String favName);

    /**
     * Rename a favorite within a group.
     *
     * @param groupName group name
     * @param oldName   current favorite name
     * @param newName   new favorite name (must be unique within group)
     * @return true if renamed, false if old not found or new name exists
     */
    public native boolean renameFavorite(String groupName, String oldName, String newName);

    /**
     * Set or clear the starred flag on a favorite.
     *
     * @param groupName group name
     * @param favName   favorite name
     * @param starred   true to star, false to unstar
     * @return true if updated, false if group or fav not found
     */
    public native boolean setStarred(String groupName, String favName, boolean starred);

    /**
     * Check if a favorite is starred.
     *
     * @param groupName group name
     * @param favName   favorite name
     * @return true if starred, false otherwise
     */
    public native boolean isStarred(String groupName, String favName);

    /**
     * Set or clear the color of a group.
     * Color is a 6-character hex RGB string (e.g. "FF5733").
     * Pass empty string to clear.
     *
     * @param groupName group name
     * @param color     6-char hex RGB string, or empty to clear
     * @return true if set, false if group not found or invalid color
     */
    public native boolean setGroupColor(String groupName, String color);

    /**
     * Get the color of a group.
     *
     * @param groupName group name
     * @return color string (6 hex chars) or empty if no color or group not found
     */
    public native String getGroupColor(String groupName);

    // ---- Map Download ----

    /** Manager for downloading maps from providers. Created lazily. */
    private MapDownloadManager mapDownloadManager;

    /**
     * Get the {@link MapDownloadManager} for downloading maps from providers.
     *
     * @return the map download manager
     */
    public MapDownloadManager getMapDownloadManager() {
        if (mapDownloadManager == null) {
            mapDownloadManager = new MapDownloadManager(this);
        }
        return mapDownloadManager;
    }
}
