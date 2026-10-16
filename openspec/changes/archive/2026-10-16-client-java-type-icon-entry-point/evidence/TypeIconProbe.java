import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

/**
 * Task 4.6 of client-java-type-icon-entry-point: a Java call that renders a real
 * type's icon from a real stylesheet, on the v27 database maps/nordrhein-westfalen
 * with stylesheets/ configured. Calls renderTypeIcon for types the shipped
 * stylesheets give an AREA.ICON, and reports the returned pixels.
 */
public class TypeIconProbe {
    private static final int SIZE = 32;

    private static void probe(OSMScoutClient c, String stylesheet, String typeName) {
        boolean loaded = c.loadStyleSheet(stylesheet);
        int[] px = c.renderTypeIcon(typeName, SIZE);
        if (px == null) {
            System.out.println("PROBE: stylesheet=" + stylesheet + " type=" + typeName
                + " loadStyleSheet=" + loaded + " -> null");
            return;
        }
        int painted = 0;
        int minX = SIZE;
        int minY = SIZE;
        int maxX = -1;
        int maxY = -1;
        int opaque = 0;
        for (int i = 0; i < px.length; i++) {
            if (px[i] != 0) {
                painted++;
                if ((px[i] >>> 24) == 0xff) {
                    opaque++;
                }
                int x = i % SIZE;
                int y = i / SIZE;
                minX = Math.min(minX, x);
                minY = Math.min(minY, y);
                maxX = Math.max(maxX, x);
                maxY = Math.max(maxY, y);
            }
        }
        System.out.println("PROBE: stylesheet=" + stylesheet + " type=" + typeName
            + " loadStyleSheet=" + loaded
            + " length=" + px.length
            + " painted=" + painted
            + " opaque=" + opaque
            + " bbox=[" + minX + "," + minY + " .. " + maxX + "," + maxY + "]"
            + " center=0x" + String.format("%08X", px[(SIZE / 2) * SIZE + SIZE / 2]));
        if (painted > 0) {
            System.out.println("PROBE:   first painted pixel=" + firstPainted(px));
        }
    }

    private static String firstPainted(int[] px) {
        for (int i = 0; i < px.length; i++) {
            if (px[i] != 0) {
                return "(" + (i % SIZE) + "," + (i / SIZE) + ")=0x" + String.format("%08X", px[i]);
            }
        }
        return "-";
    }

    public static void main(String[] args) throws Exception {
        String db = args.length > 0 ? args[0] : "maps/nordrhein-westfalen";
        String stylesheets = args.length > 1 ? args[1] : "stylesheets";
        String icons = args.length > 2 ? args[2] : "libosmscout/data/icons/14x14/standard";

        OSMScoutClient c = new OSMScoutClientBuilder()
            .withMapLookupDirectories(db)
            .withStyleSheetDirectory(stylesheets)
            .withIconDirectory(icons)
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

        System.out.println("PROBE: db=" + db + " stylesheets=" + stylesheets + " icons=" + icons
            + " size=" + SIZE + "x" + SIZE);
        probe(c, "standard", "amenity_parking");
        probe(c, "standard", "amenity_fire_station_building");
        probe(c, "railways", "railway_station");
        probe(c, "standard", "highway_roundabout");

        c.close();
    }
}
