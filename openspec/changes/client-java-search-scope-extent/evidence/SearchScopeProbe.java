import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Task 4.4 of client-java-search-scope-extent: a scoped search must not return a
 * hit from another region's free-text index, while an unscoped search does. Run
 * against the v27 database maps/nordrhein-westfalen, whose free-text index covers
 * the whole state: scoping to Dortmund's admin region must drop far-away hits.
 */
public class SearchScopeProbe {
    private static final double CITY_LAT = 51.514;
    private static final double CITY_LON = 7.465;

    private static LocationEntry[] waitForSearch(OSMScoutClient client, String query, int limit)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + 30000;
        LocationEntry[] entries = client.searchLocations(query, limit);
        while ((entries == null || entries.length == 0) && System.currentTimeMillis() < deadline) {
            Thread.sleep(50);
            entries = client.searchLocations(query, limit);
        }
        return entries;
    }

    private static LocationEntry[] stableSearch(OSMScoutClient client, String query, int limit)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + 60000;
        LocationEntry[] last = new LocationEntry[0];
        long stableSince = System.currentTimeMillis();
        while (System.currentTimeMillis() < deadline) {
            LocationEntry[] entries = client.searchLocations(query, limit);
            int count = entries == null ? 0 : entries.length;
            if (count != last.length) {
                last = entries == null ? new LocationEntry[0] : entries;
                stableSince = System.currentTimeMillis();
            } else if (count > 0 && System.currentTimeMillis() - stableSince > 4000) {
                return last;
            }
            Thread.sleep(200);
        }
        return last;
    }

    private static void report(OSMScoutClient c, long handle, String query) throws InterruptedException {
        // A new search cancels the previous one automatically, so the probes never
        // poll: they give the running search time and issue a single call each.
        Thread.sleep(1500);
        LocationEntry[] unscoped = c.searchLocations(query, 50);
        Thread.sleep(1500);
        LocationEntry[] scoped = c.searchLocations(query, 50, handle);

        int unscopedCount = unscoped == null ? -1 : unscoped.length;
        int scopedCount = scoped == null ? -1 : scoped.length;
        System.out.println("PROBE: query='" + query + "' unscoped=" + unscopedCount
            + " scoped=" + scopedCount);

        int outOfScope = 0;
        int far = 0;
        for (LocationEntry e : scoped == null ? new LocationEntry[0] : scoped) {
            double dLat = Math.abs(e.lat - CITY_LAT);
            double dLon = Math.abs(e.lon - CITY_LON);
            if (!e.inSearchScope) {
                outOfScope++;
            }
            if (dLat > 0.5 || dLon > 0.5) {
                far++;
            }
        }
        System.out.println("PROBE:   scopedOutOfScopeFlags=" + outOfScope
            + " scopedFarFromCity=" + far);

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
            for (String query : new String[]{"Bahnhof", "Dom", "Aldi"}) {
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
