package com.framstag.libosmscout;

import javafx.application.Platform;
import javafx.scene.control.ComboBox;
import javafx.scene.control.ListView;
import javafx.scene.control.Slider;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.lang.reflect.Field;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the {@link PoiSearchOverlay} UI structure: category list,
 * stepped radius slider, and result list. The overlay is constructed with a
 * {@code null} client because construction does not touch the native library;
 * search execution is covered by the native-gated integration tests.
 */
public class PoiSearchOverlayTest {

    @BeforeAll
    public static void initJavaFx() {
        try {
            Platform.startup(() -> {});
        } catch (IllegalStateException e) {
            // JavaFX runtime may already be started by another test class.
        }
    }

    private static Object getField(Object target, String name) throws Exception {
        Field field = target.getClass().getDeclaredField(name);
        field.setAccessible(true);
        return field.get(target);
    }

    @SuppressWarnings("unchecked")
    private static PoiSearchOverlay createOverlay() {
        UIScale uiScale = new UIScale();
        return new PoiSearchOverlay(null, uiScale,
            () -> 52.0, () -> 8.0,
            entry -> { },
            desc -> { });
    }

    @Test
    public void testCategoryListShowsSupportedCategories() throws Exception {
        PoiSearchOverlay overlay = createOverlay();
        ComboBox<String> combo = (ComboBox<String>) getField(overlay, "categoryCombo");

        assertEquals(14, combo.getItems().size());
        assertTrue(combo.getItems().contains("Hotels"));
        assertTrue(combo.getItems().contains("Restaurants"));
        assertTrue(combo.getItems().contains("Grocery store"));
        assertTrue(combo.getItems().contains("Viewpoint"));
        assertTrue(combo.getItems().contains("Museum"));
        assertTrue(combo.getItems().contains("Gas station"));
        assertTrue(combo.getItems().contains("Charging station"));
        assertTrue(combo.getItems().contains("ATM"));
        assertTrue(combo.getItems().contains("Tourism"));
        assertTrue(combo.getItems().contains("Parking"));
        assertTrue(combo.getItems().contains("Police station"));
        assertTrue(combo.getItems().contains("Hospital"));
        assertTrue(combo.getItems().contains("Doctors office"));
        assertTrue(combo.getItems().contains("Public transport"));
        assertEquals(0, combo.getSelectionModel().getSelectedIndex(),
            "first category should be preselected");
    }

    @Test
    public void testRadiusSliderIsStepped() throws Exception {
        PoiSearchOverlay overlay = createOverlay();
        Slider slider = (Slider) getField(overlay, "radiusSlider");

        assertTrue(slider.isSnapToTicks());
        assertEquals(1.0, slider.getMajorTickUnit());
        assertEquals(1.0, slider.getBlockIncrement());
        assertEquals(0.0, slider.getMin());
        assertEquals(5.0, slider.getMax(), "6 steps (500m..20km) map to 0..5");
    }

    @Test
    public void testResultListExists() throws Exception {
        PoiSearchOverlay overlay = createOverlay();
        ListView<?> list = (ListView<?>) getField(overlay, "resultList");
        assertNotNull(list);
        assertTrue(list.getItems().isEmpty(), "result list starts empty");
    }

    @Test
    public void testOverlayStartsHidden() throws Exception {
        PoiSearchOverlay overlay = createOverlay();
        assertTrue(overlay.isMouseTransparent(), "overlay must not block the map while closed");
    }
}
