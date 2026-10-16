import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.NavigationController;
import com.framstag.libosmscout.client.NavigationListener;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;
import com.framstag.libosmscout.client.RouteCallback;
import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.RouteInstruction;
import com.framstag.libosmscout.client.TurnType;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Task 4.9 of client-java-roundabout-instruction-data: an end-to-end navigation
 * check on a real roundabout of the v27 database maps/nordrhein-westfalen.
 *
 * Modes:
 *   search                 list location-index candidates named "Kreisverkehr"
 *   route <lat> <lon>      route across the roundabout at that position, then
 *                          navigate from its start and print the instruction data
 *                          (turn type, exit ordinal, circulation direction).
 */
public class RoundaboutProbe {
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

    private static RouteEntry route(OSMScoutClient client, double sLat, double sLon,
                                    double dLat, double dLon) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> holder = new AtomicReference<>();
        AtomicReference<String> error = new AtomicReference<>();

        client.calculateRouteAsync(sLat, sLon, dLat, dLon, new RouteCallback() {
            @Override public void onProgress(int percent) {
            }

            @Override public void onSuccess(RouteEntry r) {
                holder.set(r);
                latch.countDown();
            }

            @Override public void onError(String message) {
                error.set(message);
                latch.countDown();
            }

            @Override public void onCancel() {
                latch.countDown();
            }
        });
        if (!latch.await(90, TimeUnit.SECONDS)) {
            System.out.println("PROBE: route timeout");
            return null;
        }
        if (holder.get() == null) {
            System.out.println("PROBE: route error: " + error.get());
        }
        return holder.get();
    }

    public static void main(String[] args) throws Exception {
        String db = "maps/nordrhein-westfalen";
        String stylesheets = "stylesheets";

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

        if (args.length >= 1 && "route".equals(args[0])) {
            double lat = Double.parseDouble(args[1]);
            double lon = Double.parseDouble(args[2]);
            double d = args.length > 3 ? Double.parseDouble(args[3]) : 0.003;
            runRoute(c, lat, lon, d);
        } else {
            String[] queries = {"Kreisverkehr", "Kreisverkehr Aachenerstraße", "Nachtwächter-Kreisverkehr"};
            for (String query : queries) {
                LocationEntry[] entries = waitForSearch(c, query, 20);
                System.out.println("PROBE: query='" + query + "' results="
                    + (entries == null ? -1 : entries.length));
                if (entries == null) {
                    continue;
                }
                for (LocationEntry e : entries) {
                    System.out.println("PROBE:   label='" + e.label + "' type=" + e.type
                        + " objectType=" + e.objectType + " refType=" + e.refType
                        + " lat=" + e.lat + " lon=" + e.lon
                        + " offset=" + e.objectFileOffset);
                }
            }
        }
        c.close();
    }

    private static void runRoute(OSMScoutClient c, double lat, double lon, double d)
        throws InterruptedException {
        System.out.println("PROBE: roundabout candidate " + lat + "/" + lon + " radius=" + d);
        // Route across the candidate: from west of it to east of it.
        RouteEntry r = route(c, lat, lon - d, lat, lon + d);
        if (r == null || r.latitudes.length < 2) {
            System.out.println("PROBE: route across the roundabout failed");
            return;
        }
        System.out.println("PROBE: route distance=" + r.distance + "m duration=" + r.duration
            + "s waypoints=" + r.latitudes.length);
        System.out.println("PROBE: descriptions=" + (r.descriptions == null ? -1 : r.descriptions.length)
            + " instructionLats=" + (r.instructionLats == null ? -1 : r.instructionLats.length));

        AtomicReference<RouteInstruction[]> holder = new AtomicReference<>();
        CountDownLatch latch = new CountDownLatch(1);
        NavigationListener listener = new NavigationListener() {
            @Override
            public void onRouteInstructions(RouteInstruction[] instructions) {
                holder.set(instructions);
                latch.countDown();
            }

            @Override
            public void onNextRouteInstruction(RouteInstruction instruction) {
            }

            @Override
            public void onError(String message) {
                System.out.println("PROBE: navigation error: " + message);
            }
        };
        NavigationController controller = c.startNavigation(r.routeHandle, listener);
        controller.processLocation(r.latitudes[0], r.longitudes[0], -1.0, -1.0,
            System.currentTimeMillis());
        if (!latch.await(60, TimeUnit.SECONDS)) {
            System.out.println("PROBE: no instruction list delivered");
            controller.stop();
            return;
        }
        RouteInstruction[] list = holder.get();
        System.out.println("PROBE: instructions=" + (list == null ? -1 : list.length));
        if (list == null) {
            controller.stop();
            return;
        }
        boolean sawRoundabout = false;
        for (RouteInstruction i : list) {
            System.out.println("PROBE:   turn=" + i.turnType
                + " distanceTo=" + i.distanceTo + " legDistance=" + i.legDistance
                + " timeTo=" + i.timeTo
                + " exitNumber=" + i.roundaboutExitNumber
                + " clockwise=" + i.roundaboutClockwise
                + " nextNextExit=" + i.nextNextRoundaboutExitNumber
                + " short='" + i.shortDescription + "'");
            if (i.turnType == TurnType.ROUNDABOUT_ENTER || i.turnType == TurnType.ROUNDABOUT_LEAVE) {
                sawRoundabout = true;
            }
        }
        System.out.println("PROBE: SAW_ROUNDABOUT_INSTRUCTIONS=" + sawRoundabout);
        controller.stop();
    }
}
