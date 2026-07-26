package com.framstag.libosmscout;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for Config class.
 */
class ConfigTest {

    @Test
    void testDefaultLongPressTimeout() {
        Config config = new Config();
        int timeout = config.getLongPressTimeoutMs();
        // Default should be a reasonable positive value
        assertTrue(timeout > 0, "Default long-press timeout should be positive");
    }

    @Test
    void testSetLongPressTimeout() {
        Config config = new Config();
        config.setLongPressTimeoutMs(1000);
        assertEquals(1000, config.getLongPressTimeoutMs());
    }

    @Test
    void testSetLongPressTimeoutZero() {
        Config config = new Config();
        config.setLongPressTimeoutMs(0);
        assertEquals(0, config.getLongPressTimeoutMs());
    }

    @Test
    void testSetLongPressTimeoutNegative() {
        Config config = new Config();
        config.setLongPressTimeoutMs(-1);
        assertEquals(-1, config.getLongPressTimeoutMs());
    }

    @Test
    void testDefaultIconDirectory(@TempDir Path tempDir) {
        Path customConfig = tempDir.resolve("config.properties");
        Config config = new Config(customConfig);
        assertEquals("libosmscout/data/icons/14x14/standard", config.getIconDirectory());
    }

    @Test
    void testSetIconDirectory(@TempDir Path tempDir) {
        Path customConfig = tempDir.resolve("config.properties");
        Config config = new Config(customConfig);
        config.setIconDirectory("/custom/icons");
        assertEquals("/custom/icons", config.getIconDirectory());
    }

    @Test
    void testIconDirectoryPersistence(@TempDir Path tempDir) {
        Path customConfig = tempDir.resolve("config.properties");

        Config first = new Config(customConfig);
        first.setIconDirectory("/persisted/icons");

        Config second = new Config(customConfig);
        assertEquals("/persisted/icons", second.getIconDirectory());
    }

    @Test
    void testIconDirectoryRevertsToDefaultWhenBlank(@TempDir Path tempDir) {
        Path customConfig = tempDir.resolve("config.properties");
        Config config = new Config(customConfig);
        config.setIconDirectory("  ");
        assertEquals("libosmscout/data/icons/14x14/standard", config.getIconDirectory());
    }

    @Test
    void testLegacySvgIconDirectoryMigratedToPng(@TempDir Path tempDir) {
        Path customConfig = tempDir.resolve("config.properties");
        Config config = new Config(customConfig);
        config.setIconDirectory("libosmscout/data/icons/svg/standard");
        assertEquals("libosmscout/data/icons/14x14/standard", config.getIconDirectory());
    }
}
