import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;
import com.framstag.libosmscout.client.RouteCallback;
import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.RoutingProfile;
import com.framstag.libosmscout.client.Vehicle;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Task 3.6 of client-java-routing-profile-max-speed: observe the applied speed end to
 * end through a real route calculation on the v27 database maps/nordrhein-westfalen.
 * The bridge logs the applied speed at WARN level, which the default osmscout logger
 * writes to stderr; this probe calculates a real route per profile and prints the
 * route totals, so the log line and the route belong together.
 */
public class RoutingSpeedProbe {
    private static final double START_LAT = 51.514227;
    private static final double START_LON = 7.465279;
    private static final double DEST_LAT = 51.520;
    private static final double DEST_LON = 7.480;

    private static RouteEntry calculate(OSMScoutClient client, RoutingProfile profile)
        throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> holder = new AtomicReference<>();
        AtomicReference<String> error = new AtomicReference<>();

        client.calculateRouteWithObjectsAsync(START_LAT, START_LON, 0, null,
            DEST_LAT, DEST_LON, 0, null, profile, new RouteCallback() {
                @Override public void onProgress(int percent) {
                }

                @Override public void onSuccess(RouteEntry route) {
                    holder.set(route);
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

    private static void report(OSMScoutClient client, String label, RoutingProfile profile)
        throws InterruptedException {
        RouteEntry route = calculate(client, profile);
        if (route == null) {
            System.out.println("PROBE: " + label + " vehicle=" + profile.vehicle
                + " maxSpeedKmh=" + profile.maxSpeedKmh + " -> no route");
            return;
        }
        System.out.println("PROBE: " + label + " vehicle=" + profile.vehicle
            + " maxSpeedKmh=" + profile.maxSpeedKmh
            + " -> route distance=" + route.distance + "m duration=" + route.duration + "s"
            + " waypoints=" + route.latitudes.length);
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

        System.out.println("PROBE: db=" + db + " route " + START_LAT + "/" + START_LON
            + " -> " + DEST_LAT + "/" + DEST_LON);
        report(c, "requested-77", new RoutingProfile(Vehicle.CAR, 77.0));
        report(c, "built-in", new RoutingProfile(Vehicle.CAR));

        c.close();
    }
}
