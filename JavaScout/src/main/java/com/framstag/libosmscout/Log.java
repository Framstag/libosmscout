package com.framstag.libosmscout;

import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Simple logging utility wrapping {@link java.util.logging.Logger}.
 * <p>
 * Provides static methods for info, warning, and error logging with
 * consistent formatting. Uses java.util.logging (no extra dependencies).
 * <p>
 * Log level can be controlled via standard JUL configuration
 * (logging.properties or {@code -Djava.util.logging.config.file=...}).
 */
public final class Log {

    private static final Logger LOG = Logger.getLogger("com.framstag.libosmscout");

    private Log() {}

    /**
     * Log an info message.
     */
    public static void info(String msg) {
        LOG.info(msg);
    }

    /**
     * Log a warning message.
     */
    public static void warn(String msg) {
        LOG.warning(msg);
    }

    /**
     * Log an error message.
     */
    public static void error(String msg) {
        LOG.severe(msg);
    }

    /**
     * Log an error message with exception.
     */
    public static void error(String msg, Throwable thrown) {
        LOG.log(Level.SEVERE, msg, thrown);
    }
}
