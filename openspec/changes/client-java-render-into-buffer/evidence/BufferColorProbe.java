import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Task 5.5 of client-java-render-into-buffer, on the host surface the task allows:
 * a frame rendered through the buffer path must show the same colours as the array
 * path (the channel-swap regression). Run against the v27 database
 * maps/nordrhein-westfalen, with a route overlay so both paths draw more than the
 * plain map.
 */
public class BufferColorProbe {
    private static final int W = 64;
    private static final int H = 48;
    private static final double LAT = 51.514;
    private static final double LON = 7.465;
    private static final double MAG = 32768.0; // 2^15

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

        // A short overlay inside the view, so both paths draw the route too.
        double[] routeLats = {LAT - 0.001, LAT, LAT + 0.001};
        double[] routeLons = {LON - 0.001, LON, LON + 0.001};

        int[] arrayPath = c.renderWithRouteAndPois(W, H, LAT, LON, 0.0, MAG, Double.NaN,
            routeLats, routeLons, null, null, Double.NaN, Double.NaN, null, null);

        ByteBuffer buffer = ByteBuffer.allocateDirect(W * H * 4).order(ByteOrder.nativeOrder());
        boolean ok = c.renderInto(W, H, LAT, LON, 0.0, MAG, Double.NaN,
            routeLats, routeLons, null, null, Double.NaN, Double.NaN, null, null, buffer);

        if (arrayPath == null) {
            System.out.println("PROBE: array path returned null");
            c.close();
            return;
        }
        if (!ok) {
            System.out.println("PROBE: renderInto returned false");
            c.close();
            return;
        }

        int mismatch = 0;
        int firstBad = -1;
        int nonTransparent = 0;
        for (int i = 0; i < arrayPath.length; i++) {
            int word = arrayPath[i];
            if ((word >>> 24) != 0) {
                nonTransparent++;
            }
            byte r = buffer.get(i * 4);
            byte g = buffer.get(i * 4 + 1);
            byte b = buffer.get(i * 4 + 2);
            byte a = buffer.get(i * 4 + 3);
            int wantR = (word >> 16) & 0xff;
            int wantG = (word >> 8) & 0xff;
            int wantB = word & 0xff;
            int wantA = (word >>> 24) & 0xff;
            if ((r & 0xff) != wantR || (g & 0xff) != wantG
                || (b & 0xff) != wantB || (a & 0xff) != wantA) {
                if (firstBad < 0) {
                    firstBad = i;
                }
                mismatch++;
            }
        }

        System.out.println("PROBE: db=" + db + " size=" + W + "x" + H + " at " + LAT + "/" + LON
            + " overlay=3-waypoint route");
        System.out.println("PROBE: ARRAY_PIXELS=" + arrayPath.length
            + " BUFFER_CAPACITY=" + buffer.capacity()
            + " NON_TRANSPARENT=" + nonTransparent);
        System.out.println("PROBE: PIXELS_DIFFERING_ARRAY_VS_BUFFER=" + mismatch + "/" + arrayPath.length);
        if (firstBad >= 0) {
            int word = arrayPath[firstBad];
            System.out.println("PROBE: first mismatch at " + firstBad
                + " array=0x" + String.format("%08X", word)
                + " buffer=r" + (buffer.get(firstBad * 4) & 0xff)
                + " g" + (buffer.get(firstBad * 4 + 1) & 0xff)
                + " b" + (buffer.get(firstBad * 4 + 2) & 0xff)
                + " a" + (buffer.get(firstBad * 4 + 3) & 0xff));
        }

        c.close();
    }
}
