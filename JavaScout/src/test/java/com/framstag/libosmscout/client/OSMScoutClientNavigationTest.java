package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#startNavigation(long, NavigationListener)}.
 * Skipped automatically when the native library is not available.
 * <p>
 * The claims of this class are about an invalid route handle, so the class needs a
 * client that is actually initialised: a client without a native handle answers
 * {@code startNavigation} from its "client not initialised" guard and would make the
 * assertions below pass for a reason they do not name. The class builds one client for
 * all of its tests and releases it however the class ends.
 */
public class OSMScoutClientNavigationTest {

    private static OSMScoutClient client;

    @BeforeAll
    public static void setUp() {
        try {
            client = new OSMScoutClientBuilder()
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .build();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            // Reported per test by assumeClient() below, so a missing native library skips
            // the tests with a reason instead of dropping the class from the report.
            client = null;
        }
    }

    /**
     * The invalid-handle claims below are only about an initialised client. Probing here
     * rather than in the class setup keeps a missing native library a per-test skip with
     * its reason, and keeps an uninitialised client a failure rather than a pass.
     */
    @BeforeEach
    public void assumeClient() {
        TestClients.assumeNativeLibrary();

        assertNotNull(client, "the client of this class must be built");
        assertTrue(client.isInitialized(),
            "the class needs an initialised client, not the uninitialised guard path");
    }

    @AfterAll
    public static void tearDown() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    @Test
    public void testStartNavigationWithZeroHandleReturnsNull() {
        NavigationController controller = client.startNavigation(0, new EmptyNavigationListener());
        assertNull(controller, "zero route handle should not produce a controller");
    }

    @Test
    public void testStartNavigationWithUnknownHandleReturnsNull() {
        NavigationController controller = client.startNavigation(99999, new EmptyNavigationListener());
        assertNull(controller, "unknown route handle should not produce a controller");
    }

    private static class EmptyNavigationListener implements NavigationListener {
    }
}
