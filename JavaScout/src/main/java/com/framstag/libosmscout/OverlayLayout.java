package com.framstag.libosmscout;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;

/**
 * Shared layout logic for JavaScout floating overlay panels.
 * <p>
 * Provides DPI-aware sizing, small-screen/full-width switching, and consistent
 * bottom-right/bottom-center anchoring for {@link SearchOverlay} and {@link RoutePanel}.
 */
public class OverlayLayout {

    private final UIScale uiScale;

    public OverlayLayout(UIScale uiScale) {
        this.uiScale = uiScale;
    }

    /**
     * Compute the max width for an expanded panel given the current scene width.
     *
     * @param sceneWidth current scene width in pixels
     * @return panel max width
     */
    public double computePanelMaxWidth(double sceneWidth) {
        if (sceneWidth < uiScale.smallScreenThreshold()) {
            return Double.MAX_VALUE; // full-width mode
        }
        return uiScale.panelMaxWidth();
    }

    /**
     * Return the edge margin for overlay positioning.
     */
    public double edgeMargin() {
        return uiScale.edgeMargin();
    }

    /**
     * Anchor a collapsed overlay button bottom-right with edge margin.
     */
    public void anchorButton(StackPane overlay, Region button) {
        overlay.setAlignment(button, Pos.BOTTOM_RIGHT);
        overlay.setPadding(new Insets(0, uiScale.edgeMargin(), uiScale.edgeMargin(), 0));
    }

    /**
     * Anchor an expanded overlay panel. On small screens center it horizontally
     * at the bottom; on large screens anchor bottom-right.
     */
    public void anchorPanel(StackPane overlay, Region panel, double sceneWidth) {
        if (sceneWidth < uiScale.smallScreenThreshold()) {
            overlay.setAlignment(panel, Pos.BOTTOM_CENTER);
        } else {
            overlay.setAlignment(panel, Pos.BOTTOM_RIGHT);
        }
    }

    /**
     * Compute bottom padding for the route overlay so its collapsed button sits
     * above the search button with a scaled gap.
     *
     * @param thumbSize size of the search button
     * @return bottom insets for the route overlay
     */
    public Insets routeOverlayInsets(double thumbSize) {
        double margin = uiScale.edgeMargin();
        double gap = uiScale.buttonGap();
        return new Insets(0, margin, margin + thumbSize + gap, 0);
    }
}
