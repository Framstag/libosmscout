import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;
import com.framstag.libosmscout.client.RouteCallback;
import com.framstag.libosmscout.client.RouteEntry;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Tasks 3.2 and 3.3 of client-java-route-instruction-metrics, run on a real route of
 * the v27 database maps/nordrhein-westfalen:
 *
 *  3.2 the per-instruction arrays are index-aligned with the instruction lines, and
 *      their distances and times sum to the route's totals (the null case needs an
 *      unalignable description and cannot be forced from Java);
 *  3.3 no reported step is a single geometry edge: the smallest reported leg is larger
 *      than the largest single geometry edge of the route.
 */
public class RouteMetricsProbe {
    private static RouteEntry calculate(OSMScoutClient c, double sLat, double sLon,
                                        double dLat, double dLon) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> holder = new AtomicReference<>();
        AtomicReference<String> error = new AtomicReference<>();
        c.calculateRouteAsync(sLat, sLon, dLat, dLon, new RouteCallback() {
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

    private static boolean isInstructionLine(String line) {
        return line != null && !line.trim().isEmpty() && !line.startsWith("---");
    }

    private static double haversine(double lat1, double lon1, double lat2, double lon2) {
        final double r = 6371000.0;
        double dLat = Math.toRadians(lat2 - lat1);
        double dLon = Math.toRadians(lon2 - lon1);
        double a = Math.sin(dLat / 2) * Math.sin(dLat / 2)
            + Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2))
            * Math.sin(dLon / 2) * Math.sin(dLon / 2);
        return 2 * r * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    }

    private static void analyse(OSMScoutClient c, String label, double sLat, double sLon,
                                double dLat, double dLon) throws InterruptedException {
        RouteEntry r = calculate(c, sLat, sLon, dLat, dLon);
        System.out.println("PROBE: === " + label + " " + sLat + "/" + sLon + " -> " + dLat + "/" + dLon);
        if (r == null) {
            System.out.println("PROBE: no route");
            return;
        }
        int lines = 0;
        if (r.descriptions != null) {
            for (String line : r.descriptions) {
                System.out.println("PROBE: desc: " + line);
                if (isInstructionLine(line)) {
                    lines++;
                }
            }
        }
        System.out.println("PROBE: distance=" + r.distance + " duration=" + r.duration
            + " waypoints=" + r.latitudes.length
            + " descriptions=" + (r.descriptions == null ? -1 : r.descriptions.length)
            + " instructionLines=" + lines);

        boolean aligned = r.instructionLats != null && r.instructionLons != null
            && r.instructionDistances != null && r.instructionTimes != null;
        System.out.println("PROBE: ARRAYS_NON_NULL=" + aligned);
        if (!aligned) {
            return;
        }
        System.out.println("PROBE: arrayLengths=" + r.instructionLats.length + ","
            + r.instructionLons.length + "," + r.instructionDistances.length + ","
            + r.instructionTimes.length);
        System.out.println("PROBE: ALIGNED_WITH_INSTRUCTION_LINES=" + (r.instructionLats.length == lines));

        double sumDist = 0.0;
        double sumTime = 0.0;
        for (double d : r.instructionDistances) {
            sumDist += d;
        }
        for (double t : r.instructionTimes) {
            sumTime += t;
        }
        System.out.println("PROBE: sumDistances=" + sumDist + " diffVsDistance=" + (sumDist - r.distance));
        System.out.println("PROBE: sumTimes=" + sumTime + " diffVsDuration=" + (sumTime - r.duration));

        double maxEdge = 0.0;
        for (int i = 1; i < r.latitudes.length; i++) {
            maxEdge = Math.max(maxEdge, haversine(r.latitudes[i - 1], r.longitudes[i - 1],
                r.latitudes[i], r.longitudes[i]));
        }
        double minLeg = Double.POSITIVE_INFINITY;
        for (double d : r.instructionDistances) {
            if (d > 0.0) {
                minLeg = Math.min(minLeg, d);
            }
        }
        System.out.println("PROBE: maxSingleGeometryEdge=" + maxEdge
            + " minReportedLeg=" + minLeg
            + " MIN_LEG_LARGER_THAN_MAX_EDGE=" + (minLeg > maxEdge));
        System.out.println("PROBE: legs=" + java.util.Arrays.toString(r.instructionDistances));
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
        if (args.length >= 7 && "pair".equals(args[2])) {
            analyse(c, "pair",
                Double.parseDouble(args[3]), Double.parseDouble(args[4]),
                Double.parseDouble(args[5]), Double.parseDouble(args[6]));
            c.close();
            return;
        }
        analyse(c, "city", 51.514227, 7.465279, 51.520, 7.480);
        analyse(c, "longer", 51.514227, 7.465279, 51.500, 7.500);
        c.close();
    }
}
