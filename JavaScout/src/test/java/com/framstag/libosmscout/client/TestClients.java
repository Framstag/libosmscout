package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;

/**
 * Helpers shared by the JNI integration tests.
 * <p>
 * The native layer supports a single active client at a time, so a client this
 * class builds is a probe only: it is released immediately and is never handed to
 * an assertion. A test that needs a client of its own builds one and releases it
 * however the test ends - see the class comments of the tests that use it.
 */
final class TestClients {

    private TestClients() {
    }

    /**
     * Skip the test when the native library cannot be loaded. Building a client
     * detects a missing library, so the probe is released again right away and is
     * never used as the subject of an assertion.
     */
    static void assumeNativeLibrary() {
        OSMScoutClient probe = null;

        try {
            probe = new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        } finally {
            if (probe != null) {
                // A probe without a native handle: nothing to release, but closing it
                // keeps the rule "whatever this class builds, it releases" uniform.
                probe.close();
            }
        }
    }

    /**
     * A receiver for a JNI call that does not read its receiver. It carries no native
     * handle, does not occupy the process-wide client slot and must never be the subject
     * of an assertion: build a real client for that. The caller releases it.
     */
    static OSMScoutClient receiverWithoutNativeHandle() {
        return new OSMScoutClient();
    }
}
