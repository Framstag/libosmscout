import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

import java.util.Arrays;

/**
 * Task 4.4 of client-java-search-scope-extent: a scoped search must not return a
 * hit from another region's free-text index, while an unscoped search does. Run
 * against the v27 database maps/nordrhein-westfalen, whose free-text index covers
 * the whole state: scoping to Dortmund's admin region must drop far-away hits.
 *
 * Updated 2026-10-27 for the API of `client-java-search-request-lifecycle`: the
 * scoped call carries a generation from `beginSearch` instead of the plain
 * three-argument overload the 2026-10-16 run used. Always run with `2>&1`: the
 * bridge reports an extent it could not establish on stderr ("search scope: no
 * extent for region ... - filtering disabled"), and that line is what separates
 * "the filter never ran" from "the filter ran and admitted the hit".
 */
public class SearchScopeProbe {
    private static final double CITY_LAT = 51.514;
    private static final double CITY_LON = 7.465;

    private static LocationEntry[] scopedSearch(OSMScoutClient c, String query, int limit, long handle) {
        final long generation = c.beginSearch(OSMScoutClient.SEARCH_SLOT_PLACES);
        try {
            return c.searchLocations(query, limit, handle, generation);
        } finally {
            c.cancelSearch(generation);
        }
    }

    private static void report(OSMScoutClient c, long handle, String query) throws InterruptedException {
        // A new search cancels the previous one automatically, so the probes never
        // poll: they give the running search time and issue a single call each.
        // The first call of a query pair loads the text index and does not return its
        // hits (observed 2026-10-27: the first call returns the structured set only),
        // so it is issued and discarded, and both measured calls follow it.
        Thread.sleep(1500);
        c.searchLocations(query, 50);
        Thread.sleep(1500);
        LocationEntry[] unscoped = c.searchLocations(query, 50);
        Thread.sleep(1500);
        LocationEntry[] unscoped2 = c.searchLocations(query, 50);
        Thread.sleep(1500);
        LocationEntry[] scoped = scopedSearch(c, query, 50, handle);
        Thread.sleep(1500);
        LocationEntry[] scoped2 = scopedSearch(c, query, 50, handle);
        Thread.sleep(1500);
        // Same native entry point as the scoped call, with no region at all: this
        // separates the Java 2-argument convenience path from the bridge's
        // free-text pass (2026-10-27: the convenience path returns no free-text hit).
        LocationEntry[] nativeUnscoped = scopedSearch(c, query, 50, OSMScoutClient.NO_ADMIN_REGION);

        int unscopedCount = unscoped == null ? -1 : unscoped.length;
        int scopedCount = scoped == null ? -1 : scoped.length;
        System.out.println("PROBE: query='" + query + "' unscoped=" + unscopedCount
            + " (repeat " + (unscoped2 == null ? -1 : unscoped2.length) + ")"
            + " scoped=" + scopedCount
            + " (repeat " + (scoped2 == null ? -1 : scoped2.length) + ")"
            + " handle0=" + (nativeUnscoped == null ? -1 : nativeUnscoped.length));

        int outOfScope = 0;
        int far = 0;
        double minLat = Double.MAX_VALUE, maxLat = -Double.MAX_VALUE;
        double minLon = Double.MAX_VALUE, maxLon = -Double.MAX_VALUE;
        double uminLat = Double.MAX_VALUE, umaxLat = -Double.MAX_VALUE;
        double uminLon = Double.MAX_VALUE, umaxLon = -Double.MAX_VALUE;
        for (LocationEntry e : unscoped == null ? new LocationEntry[0] : unscoped) {
            uminLat = Math.min(uminLat, e.lat);
            umaxLat = Math.max(umaxLat, e.lat);
            uminLon = Math.min(uminLon, e.lon);
            umaxLon = Math.max(umaxLon, e.lon);
        }
        for (LocationEntry e : scoped == null ? new LocationEntry[0] : scoped) {
            double dLat = Math.abs(e.lat - CITY_LAT);
            double dLon = Math.abs(e.lon - CITY_LON);
            if (!e.inSearchScope) {
                outOfScope++;
            }
            if (dLat > 0.5 || dLon > 0.5) {
                far++;
            }
            minLat = Math.min(minLat, e.lat);
            maxLat = Math.max(maxLat, e.lat);
            minLon = Math.min(minLon, e.lon);
            maxLon = Math.max(maxLon, e.lon);
        }
        System.out.println("PROBE:   scopedOutOfScopeFlags=" + outOfScope
            + " scopedFarFromCity=" + far);
        if (scopedCount > 0) {
            System.out.println("PROBE:   scopedSpan lat=" + minLat + ".." + maxLat
                + " lon=" + minLon + ".." + maxLon);
        }
        if (unscopedCount > 0) {
            System.out.println("PROBE:   unscopedSpan lat=" + uminLat + ".." + umaxLat
                + " lon=" + uminLon + ".." + umaxLon);
        }

        // How far do the unscoped hits reach?
        double maxDist = 0.0;
        for (LocationEntry e : unscoped == null ? new LocationEntry[0] : unscoped) {
            maxDist = Math.max(maxDist, Math.hypot(e.lat - CITY_LAT, e.lon - CITY_LON));
        }
        System.out.println("PROBE:   unscopedMaxDistanceDeg=" + maxDist);

        // Are the scoped hits a subset of the unscoped hits (same label+position)?
        int notInUnscoped = 0;
        for (LocationEntry s : scoped == null ? new LocationEntry[0] : scoped) {
            boolean found = false;
            for (LocationEntry u : unscoped == null ? new LocationEntry[0] : unscoped) {
                if (u.lat == s.lat && u.lon == s.lon
                    && (u.label == null ? s.label == null : u.label.equals(s.label))) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                notInUnscoped++;
            }
        }
        System.out.println("PROBE:   scopedNotInUnscoped=" + notInUnscoped);

        // Hits the text index finds but a scoped search does not return because they lie
        // outside the scope's extent: the unscoped free-text hits outside the scoped span.
        int freeTextUnscoped = 0;
        int droppedByScope = 0;
        for (LocationEntry u : unscoped == null ? new LocationEntry[0] : unscoped) {
            if (!"freeText".equals(u.matchedComponent)) {
                continue;
            }
            freeTextUnscoped++;
            if (scopedCount > 0 && (u.lat < minLat || u.lat > maxLat
                                    || u.lon < minLon || u.lon > maxLon)) {
                droppedByScope++;
            }
        }
        System.out.println("PROBE:   unscopedFreeText=" + freeTextUnscoped
            + " outsideScopedSpan=" + droppedByScope);

        for (LocationEntry e : unscoped == null ? new LocationEntry[0] : unscoped) {
            System.out.println("PROBE:   unscoped label='" + e.label + "' refType=" + e.refType
                + " objectType=" + e.objectType + " matchQuality=" + e.matchQuality
                + " lat=" + e.lat + " lon=" + e.lon + " inScope=" + e.inSearchScope);
        }
        for (LocationEntry e : scoped == null ? new LocationEntry[0] : scoped) {
            System.out.println("PROBE:   scoped label='" + e.label + "' type=" + e.type
                + " refType=" + e.refType + " objectType=" + e.objectType
                + " lat=" + e.lat + " lon=" + e.lon + " inScope=" + e.inSearchScope);
        }
    }

    public static void main(String[] args) throws Exception {
        String db = args.length > 0 ? args[0] : "maps/nordrhein-westfalen";
        String stylesheets = args.length > 1 ? args[1] : "stylesheets";

        OSMScoutClient c = new OSMScoutClientBuilder()
            .withMapLookupDirectories(db)
            .withStyleSheetDirectory(stylesheets)
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        if (c == null) {
            System.out.println("PROBE: no client");
            return;
        }
        if (!c.openDatabase(db)) {
            System.out.println("PROBE: database did not open: " + db);
            return;
        }

        long handle = 0;
        try {
            // Trigger the text-index load once, then let it finish before measuring:
            // a polled search cancels the previous one and would only ever return the
            // structured part.
            c.searchLocations("Dortmund", 20);
            Thread.sleep(15000);
            handle = c.resolveAdminRegion(CITY_LAT, CITY_LON);
            System.out.println("PROBE: db=" + db + " city=" + CITY_LAT + "/" + CITY_LON
                + " handle=" + handle
                + " regionName=" + c.getAdminRegionName(handle)
                + " scopeName=" + c.getAdminRegionScopeName(handle));
            if (handle == 0) {
                System.out.println("PROBE: no admin region resolved - scope not testable");
                return;
            }
            for (String query : args.length > 2
                     ? Arrays.copyOfRange(args, 2, args.length)
                     : new String[]{"Bahnhof", "Dom", "Aldi"}) {
                report(c, handle, query);
            }
        } finally {
            if (handle != 0) {
                c.releaseAdminRegion(handle);
            }
            c.close();
        }
    }
}
