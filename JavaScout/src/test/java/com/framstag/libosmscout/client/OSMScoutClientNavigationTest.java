package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#startNavigation(long, NavigationListener)}.
 * Skipped automatically when the native library is not available.
 */
public class OSMScoutClientNavigationTest {

    private static OSMScoutClient client;

    @BeforeAll
    public static void setUp() {
        try {
            client = new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
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
