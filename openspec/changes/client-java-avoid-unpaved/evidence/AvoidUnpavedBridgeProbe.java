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
 * Task 4.1 of client-java-avoid-unpaved: observe the flag end to end through the JNI bridge.
 *
 * The probe calculates the same route twice against one imported database — once with
 * {@code avoidUnpaved = false} and once with {@code true} — and prints both totals. It answers two
 * questions the C++ probe cannot: that the flag reaches CreateRoutingProfile through
 * calculateRouteAsync at all, and that a profile carrying the graded table still calculates a route
 * (a table that prices degraded roads too low makes the routing search report no route).
 *
 * The bridge logs the applied table at WARN level, which the default logger writes to stderr; the log
 * line and the numbers belong together.
 *
 * Run:
 *   java -Djava.library.path=<build>/libosmscout-client-java/src \
 *        -cp <build>/libosmscout-client-java/java/libosmscoutclientjava.jar:. \
 *        AvoidUnpavedBridgeProbe <map dir> <stylesheets dir>
 */
public class AvoidUnpavedBridgeProbe {

    // A pair the C++ probe routes on the bremen-latest extract.
    private static final double START_LAT = 53.03;
    private static final double START_LON = 8.80;
    private static final double DEST_LAT = 53.06;
    private static final double DEST_LON = 8.83;

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
            System.err.println("usage: AvoidUnpavedBridgeProbe <map dir> <stylesheets dir>");
            System.exit(2);
        }

        String mapDir = args[0];
        String stylesheetsDir = args[1];

        OSMScoutClient client = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir)
            .withStyleSheetDirectory(stylesheetsDir)
            .build();

        try {
            boolean[] opened = client.openDatabases(new String[]{mapDir});

            if (opened.length == 0 || !opened[0]) {
                throw new IllegalStateException("cannot open the database at " + mapDir);
            }

            RoutingProfile plain = new RoutingProfile(Vehicle.CAR, false, false, false, 120.0);
            RoutingProfile avoidUnpaved = new RoutingProfile(Vehicle.CAR, false, false, true, 120.0);

            RouteEntry plainRoute = calculate(client, plain);
            RouteEntry unpavedRoute = calculate(client, avoidUnpaved);

            System.out.println("PROBE: avoidUnpaved=false distance=" + plainRoute.distance
                + "m duration=" + plainRoute.duration + "s points=" + plainRoute.latitudes.length);
            System.out.println("PROBE: avoidUnpaved=true  distance=" + unpavedRoute.distance
                + "m duration=" + unpavedRoute.duration + "s points=" + unpavedRoute.latitudes.length);
            System.out.println("PROBE: route differs="
                + (plainRoute.distance != unpavedRoute.distance
                   || plainRoute.latitudes.length != unpavedRoute.latitudes.length));
        } finally {
            client.close();
        }
    }
}
