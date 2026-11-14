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
 * Task 4.2 of client-java-avoid-tolls: the flag end to end through the JNI bridge.
 *
 * Calculates the same route twice on the hand-written toll test map — once with avoidTolls false, once
 * with true — and prints both routes. Without the flag the short toll way is taken; with it the free
 * detour must be taken, and both calculations must return a route.
 */
public class AvoidTollsBridgeProbe {

    // The two far nodes of the test map: A (50.0000,8.0000) and B (50.0000,8.0500).
    private static final double START_LAT = 50.0000;
    private static final double START_LON = 8.0000;
    private static final double DEST_LAT = 50.0000;
    private static final double DEST_LON = 8.0500;

    private static RouteEntry calculate(OSMScoutClient client, RoutingProfile profile)
        throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> holder = new AtomicReference<>();
        AtomicReference<String> error = new AtomicReference<>();

        client.calculateRouteAsync(START_LAT, START_LON, DEST_LAT, DEST_LON, profile,
            new RouteCallback() {
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
                    error.set("cancelled");
                    latch.countDown();
                }
            });

        if (!latch.await(120, TimeUnit.SECONDS)) {
            throw new IllegalStateException("route calculation timed out");
        }

        if (error.get() != null) {
            throw new IllegalStateException("route calculation failed: " + error.get());
        }

        return holder.get();
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.err.println("usage: AvoidTollsBridgeProbe <map dir> <stylesheets dir>");
            System.exit(2);
        }

        OSMScoutClient client = new OSMScoutClientBuilder()
            .withMapLookupDirectories(args[0])
            .withStyleSheetDirectory(args[1])
            .build();

        try {
            boolean[] opened = client.openDatabases(new String[]{args[0]});

            if (opened.length == 0 || !opened[0]) {
                throw new IllegalStateException("cannot open the database at " + args[0]);
            }

            RoutingProfile plain = new RoutingProfile(Vehicle.CAR, false, false, false, 120.0);
            RoutingProfile avoidTolls = new RoutingProfile(Vehicle.CAR, true, false, false, 120.0);

            RouteEntry plainRoute = calculate(client, plain);
            RouteEntry tollRoute = calculate(client, avoidTolls);

            System.out.println("PROBE: avoidTolls=false points=" + plainRoute.latitudes.length
                + " duration=" + plainRoute.duration + "s");
            System.out.println("PROBE: avoidTolls=true  points=" + tollRoute.latitudes.length
                + " duration=" + tollRoute.duration + "s");
            System.out.println("PROBE: route differs="
                + (plainRoute.latitudes.length != tollRoute.latitudes.length));
        } finally {
            client.close();
        }
    }
}
