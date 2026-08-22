package com.framstag.libosmscout;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;

/**
 * Shared 3-line cell layout for search results and the long-press candidate picker.
 * <p>
 * Extracted from {@link SearchOverlay} so candidate entries use the identical
 * description format as search results.
 */
public final class SearchResultCell {

    private SearchResultCell() {
    }

    /**
     * Create the 3-line search result cell with a right-aligned distance label
     * on the primary line.
     *
     * @param uiScale    DPI-aware UI scale helper
     * @param line1      primary line (label/name)
     * @param line2      secondary line (e.g. admin hierarchy), may be null or empty
     * @param line3      tertiary line (e.g. object type + offset), may be null or empty
     * @param distanceKm distance to display right-aligned, may be null to omit
     * @return cell VBox
     */
    public static VBox create(UIScale uiScale, String line1, String line2, String line3, String distanceKm) {
        VBox box = new VBox(uiScale.px(2));
        box.setPadding(new Insets(uiScale.px(4), uiScale.px(6), uiScale.px(4), uiScale.px(6)));

        if (distanceKm != null && !distanceKm.isEmpty()) {
            HBox line1Box = new HBox(uiScale.px(6));
            line1Box.setAlignment(Pos.CENTER_LEFT);

            Label label1 = new Label(line1 != null ? line1 : "");
            label1.getStyleClass().add("search-result-line1");
            label1.setMaxWidth(Double.MAX_VALUE);
            HBox.setHgrow(label1, Priority.ALWAYS);

            Label distanceLabel = new Label(distanceKm);
            distanceLabel.getStyleClass().add("search-result-distance");

            line1Box.getChildren().addAll(label1, distanceLabel);
            box.getChildren().add(line1Box);
        } else {
            Label label1 = new Label(line1 != null ? line1 : "");
            label1.getStyleClass().add("search-result-line1");
            box.getChildren().add(label1);
        }

        if (line2 != null && !line2.isEmpty()) {
            Label label2 = new Label(line2);
            label2.getStyleClass().add("search-result-line2");
            box.getChildren().add(label2);
        }

        if (line3 != null && !line3.isEmpty()) {
            Label label3 = new Label(line3);
            label3.getStyleClass().add("search-result-line3");
            box.getChildren().add(label3);
        }

        return box;
    }

    /**
     * Create the 3-line search result cell.
     *
     * @param uiScale DPI-aware UI scale helper
     * @param line1   primary line (label/name)
     * @param line2   secondary line (e.g. admin hierarchy), may be null or empty
     * @param line3   tertiary line (e.g. object type + offset), may be null or empty
     * @return cell VBox
     */
    public static VBox create(UIScale uiScale, String line1, String line2, String line3) {
        return create(uiScale, line1, line2, line3, null);
    }
}
