import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

/**
 * Task 3.3 of client-java-render-dpi, redone against the v27 database
 * maps/nordrhein-westfalen (real map, real type set). A request with an explicit
 * DPI must project at that DPI; a request without one must project with the DPI
 * configured on the client, and a stated DPI must not change what a later
 * no-DPI request projects with.
 */
public class DpiProbe {
    private static final int W = 64;
    private static final int H = 48;
    private static final double LAT = 51.514;
    private static final double LON = 7.465;
    private static final double MAG = 32768.0; // 2^15

    private static int differing(int[] a, int[] b) {
        int n = 0;
        for (int i = 0; i < a.length; i++) {
            if (a[i] != b[i]) {
                n++;
            }
        }
        return n;
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
            System.out.println("PROBE: no client (client already initialised)");
            return;
        }
        if (!c.openDatabase(db)) {
            System.out.println("PROBE: database did not open: " + db);
            return;
        }

        int[] noDpi = c.render(W, H, LAT, LON, 0.0, MAG);
        int[] p96 = c.render(W, H, LAT, LON, 0.0, MAG, 96.0);
        int[] p400 = c.render(W, H, LAT, LON, 0.0, MAG, 400.0);
        int[] pNaN = c.render(W, H, LAT, LON, 0.0, MAG, Double.NaN);
        int[] pNeg = c.render(W, H, LAT, LON, 0.0, MAG, -1.0);
        int[] again = c.render(W, H, LAT, LON, 0.0, MAG);

        System.out.println("PROBE: db=" + db + " stylesheets=" + stylesheets
            + " size=" + W + "x" + H + " at " + LAT + "/" + LON + " mag=" + MAG);
        System.out.println("PROBE: RENDER_NO_DPI=" + (noDpi == null ? -1 : noDpi.length)
            + " P96=" + (p96 == null ? -1 : p96.length)
            + " P400=" + (p400 == null ? -1 : p400.length)
            + " PNaN=" + (pNaN == null ? -1 : pNaN.length)
            + " PNEG=" + (pNeg == null ? -1 : pNeg.length)
            + " P_NO_DPI_AGAIN=" + (again == null ? -1 : again.length));
        if (noDpi == null || p96 == null || p400 == null || pNaN == null || pNeg == null || again == null) {
            System.out.println("PROBE: a frame was null - contract not observable");
            c.close();
            return;
        }
        System.out.println("PROBE: NON_TRANSPARENT_NO_DPI=" + nonTransparent(noDpi));
        System.out.println("PROBE: NO_DPI_EQUALS_96=" + (differing(noDpi, p96) == 0));
        System.out.println("PROBE: NAN_EQUALS_96=" + (differing(pNaN, p96) == 0));
        System.out.println("PROBE: NEG_EQUALS_96=" + (differing(pNeg, p96) == 0));
        System.out.println("PROBE: NO_DPI_STILL_96_AFTER_400=" + (differing(again, p96) == 0));
        System.out.println("PROBE: PIXELS_DIFFERING_96_VS_400=" + differing(p96, p400) + "/" + p96.length);
        System.out.println("PROBE: PIXELS_DIFFERING_NO_DPI_VS_400=" + differing(noDpi, p400) + "/" + noDpi.length);

        c.close();
    }

    private static int nonTransparent(int[] frame) {
        int n = 0;
        for (int word : frame) {
            if ((word >>> 24) != 0) {
                n++;
            }
        }
        return n;
    }
}
