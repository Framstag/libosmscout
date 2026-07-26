package com.framstag.libosmscout;

import javafx.geometry.Insets;
import javafx.stage.Screen;

/**
 * DPI-aware scaling helper for JavaScout UI elements.
 * <p>
 * Converts design units (based on a 96dpi desktop reference) into actual
 * pixel values using the primary screen DPI. Provides constants for touch
 * targets ("thumb on display") and panel sizing.
 */
public class UIScale {

    private static final double REFERENCE_DPI = 96.0;
    private static final double MIN_SCALE = 1.0;
    private static final double MAX_SCALE = 2.0;

    private static final double THUMB_MM = 10.0;
    private static final double MM_TO_INCH = 1.0 / 25.4;

    private static final double DESIGN_BUTTON_SIZE_DP = 40.0;
    private static final double DESIGN_PANEL_MAX_WIDTH_DP = 500.0;
    private static final double DESIGN_PANEL_SMALL_MAX_WIDTH_DP = 360.0;
    private static final double DESIGN_GAP_DP = 4.0;
    private static final double DESIGN_EDGE_MARGIN_DP = 8.0;
    private static final double SMALL_SCREEN_DP = 600.0;

    // Font scale: use a single reference font size and derive sizes from it.
    private static final double BASE_FONT_SIZE_DP = 14.0;
    private static final double BODY_FONT_SIZE_DP = 12.0;
    private static final double SMALL_FONT_SIZE_DP = 10.0;

    private final double scale;

    /**
     * Create a scale helper from the primary screen DPI.
     */
    public UIScale() {
        double dpi = REFERENCE_DPI;
        Screen screen = Screen.getPrimary();
        if (screen != null) {
            dpi = screen.getDpi();
        }
        this.scale = Math.max(MIN_SCALE, Math.min(MAX_SCALE, dpi / REFERENCE_DPI));
    }

    /**
     * Convert a design-unit value (dp) to pixels for the current display.
     */
    public double px(double designDp) {
        return Math.round(designDp * scale);
    }

    /**
     * Touch-friendly button size: at least 10mm on the physical display,
     * never smaller than the desktop reference size.
     */
    public double thumbSize() {
        double mmSize = THUMB_MM * MM_TO_INCH * REFERENCE_DPI * scale;
        return Math.max(px(DESIGN_BUTTON_SIZE_DP), mmSize);
    }

    /**
     * Gap between stacked overlay buttons, scaled with thumb size.
     */
    public double buttonGap() {
        return thumbSize() * 0.25;
    }

    /**
     * Standard edge margin for floating overlays.
     */
    public double edgeMargin() {
        return px(DESIGN_EDGE_MARGIN_DP);
    }

    /**
     * Create uniform insets from design dp.
     */
    public Insets insets(double designDp) {
        double v = px(designDp);
        return new Insets(v, v, v, v);
    }

    /**
     * Maximum width for an expanded overlay panel on large screens.
     */
    public double panelMaxWidth() {
        return px(DESIGN_PANEL_MAX_WIDTH_DP);
    }

    /**
     * Scene width threshold below which panels switch to full-width mode.
     */
    public double smallScreenThreshold() {
        return px(SMALL_SCREEN_DP);
    }

    /**
     * Maximum panel width on small screens (fractional fallback).
     */
    public double panelSmallMaxWidth() {
        return px(DESIGN_PANEL_SMALL_MAX_WIDTH_DP);
    }

    /**
     * Base font size for primary controls (text fields, buttons, labels).
     */
    public double baseFontSize() {
        return px(BASE_FONT_SIZE_DP);
    }

    /**
     * Body font size for secondary content (result descriptions, route details).
     */
    public double bodyFontSize() {
        return px(BODY_FONT_SIZE_DP);
    }

    /**
     * Small font size for tertiary metadata (object offsets, hints).
     */
    public double smallFontSize() {
        return px(SMALL_FONT_SIZE_DP);
    }

    /**
     * Standard control height matching base font size for aligned buttons/fields.
     */
    public double controlHeight() {
        return px(BASE_FONT_SIZE_DP + 14);
    }

    /**
     * Standard vertical gap between major blocks inside a panel.
     */
    public double blockGap() {
        return px(8);
    }
}
