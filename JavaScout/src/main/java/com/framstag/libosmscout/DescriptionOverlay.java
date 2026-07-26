package com.framstag.libosmscout;

import com.framstag.libosmscout.client.DescriptionEntry;
import com.framstag.libosmscout.client.ObjectDescription;

import javafx.animation.FadeTransition;
import javafx.animation.ParallelTransition;
import javafx.animation.TranslateTransition;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.util.Duration;

import java.util.List;

/**
 * Overlay dialog that displays a structured {@link ObjectDescription}.
 * <p>
 * Visually consistent with {@link SearchOverlay}: same panel styling,
 * fade/slide animation, fullscreen on small screens, click-outside
 * or Escape to close. Content is rendered dynamically from the
 * section/subsection/label/value structure of the description entries.
 */
public class DescriptionOverlay extends StackPane {

    private static final double SMALL_SCREEN_THRESHOLD = 600.0;

    private final UIScale uiScale;
    private final VBox contentBox;
    private final ScrollPane scrollPane;
    private final VBox mainPanel;
    private final StackPane contentArea;

    private boolean expanded = false;

    /**
     * Create a description overlay with the given object description.
     *
     * @param description the structured description to display
     */
    public DescriptionOverlay(ObjectDescription description) {
        this(new UIScale(), description);
    }

    /**
     * Create a description overlay with explicit UI scale.
     *
     * @param uiScale     the DPI-aware UI scale helper
     * @param description the structured description to display
     */
    public DescriptionOverlay(UIScale uiScale, ObjectDescription description) {
        this.uiScale = uiScale;

        double controlHeight = uiScale.controlHeight();
        double baseFont = uiScale.baseFontSize();
        double bodyFont = uiScale.bodyFontSize();
        double smallFont = uiScale.smallFontSize();
        double margin = uiScale.edgeMargin();
        double blockGap = uiScale.blockGap();

        // Semi-transparent overlay background
        StackPane overlay = new StackPane();
        overlay.setStyle("-fx-background-color: rgba(0,0,0,0.2);");
        StackPane.setAlignment(overlay, Pos.TOP_LEFT);

        // Main dialog panel — reuse search-overlay-panel styling
        mainPanel = new VBox(blockGap);
        mainPanel.getStyleClass().add("search-overlay-panel");
        mainPanel.setMaxWidth(uiScale.panelMaxWidth());
        mainPanel.setMaxHeight(uiScale.px(600));

        // Title bar
        HBox titleBar = new HBox(uiScale.px(8));
        titleBar.setAlignment(Pos.CENTER_LEFT);
        titleBar.setStyle("-fx-background-color: #f0f0f0; -fx-border-color: #ddd; -fx-border-width: 0 0 1px 0;");
        titleBar.setPadding(uiScale.insets(8));

        Label titleLabel = new Label("Object Description");
        titleLabel.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold;");
        titleLabel.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(titleLabel, Priority.ALWAYS);

        Button closeButton = new Button("\u2715");
        closeButton.setStyle("-fx-background-color: transparent; -fx-text-fill: #999; -fx-font-size: " + baseFont + "px; -fx-cursor: hand;");
        closeButton.setMinSize(controlHeight, controlHeight);
        closeButton.setPrefSize(controlHeight, controlHeight);
        closeButton.setMaxSize(controlHeight, controlHeight);
        closeButton.setOnAction(e -> close());

        titleBar.getChildren().addAll(titleLabel, closeButton);

        // Content area with scroll
        contentBox = new VBox(uiScale.px(4));
        contentBox.setPadding(new Insets(uiScale.px(8), uiScale.px(12), uiScale.px(12), uiScale.px(12)));

        scrollPane = new ScrollPane(contentBox);
        scrollPane.setFitToWidth(true);
        scrollPane.setHbarPolicy(ScrollPane.ScrollBarPolicy.NEVER);
        scrollPane.setStyle("-fx-background-color: transparent; -fx-border-color: transparent;");
        VBox.setVgrow(scrollPane, Priority.ALWAYS);

        // Populate content
        populateContent(description, baseFont, bodyFont, smallFont);

        mainPanel.getChildren().addAll(titleBar, scrollPane);

        // Content area wraps mainPanel for click-outside detection
        contentArea = new StackPane(mainPanel);
        StackPane.setAlignment(mainPanel, Pos.CENTER);
        StackPane.setMargin(mainPanel, new Insets(uiScale.px(40), margin, margin, margin));

        // Click outside to close
        contentArea.setOnMouseClicked(e -> {
            double x = e.getX();
            double y = e.getY();
            double panelX = (contentArea.getWidth() - mainPanel.getWidth()) / 2.0;
            double panelY = (contentArea.getHeight() - mainPanel.getHeight()) / 2.0;
            double panelW = mainPanel.getWidth();
            double panelH = mainPanel.getHeight();
            if (x < panelX || x > panelX + panelW || y < panelY || y > panelY + panelH) {
                close();
            }
        });

        // Escape to close
        contentArea.addEventHandler(KeyEvent.KEY_PRESSED, e -> {
            if (e.getCode() == KeyCode.ESCAPE) {
                close();
            }
        });
        contentArea.setFocusTraversable(true);

        getChildren().addAll(overlay, contentArea);

        // Initial state: invisible
        setVisible(false);
        setOpacity(0.0);
    }

    private void populateContent(ObjectDescription description,
                                 double baseFont, double bodyFont, double smallFont) {
        List<DescriptionEntry> entries = description.getEntries();
        if (entries.isEmpty()) {
            Label emptyLabel = new Label("No description available");
            emptyLabel.setStyle("-fx-font-size: " + bodyFont + "px; -fx-text-fill: #999; -fx-padding: " + uiScale.px(20) + "px;");
            contentBox.getChildren().add(emptyLabel);
            return;
        }

        String currentSection = null;
        String currentSubsection = null;
        int currentIndex = -1;

        for (DescriptionEntry entry : entries) {
            // Close previous section if section changes
            if (currentSection != null && !entry.sectionKey.equals(currentSection)) {
                currentSubsection = null;
                currentIndex = -1;
            }

            // Open new section
            if (!entry.sectionKey.equals(currentSection)) {
                Label sectionLabel = new Label(entry.sectionKey);
                sectionLabel.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold; -fx-text-fill: #333; -fx-padding: " + uiScale.px(8) + "px 0 " + uiScale.px(2) + "px 0;");
                contentBox.getChildren().add(sectionLabel);
                currentSection = entry.sectionKey;
                currentSubsection = null;
                currentIndex = -1;
            }

            // Open new subsection
            if (entry.subsectionKey != null && !entry.subsectionKey.isEmpty() && !entry.subsectionKey.equals(currentSubsection)) {
                Label subLabel = new Label(entry.subsectionKey);
                subLabel.setStyle("-fx-font-size: " + bodyFont + "px; -fx-font-weight: bold; -fx-text-fill: #555; -fx-padding: " + uiScale.px(4) + "px 0 " + uiScale.px(2) + "px " + uiScale.px(12) + "px;");
                contentBox.getChildren().add(subLabel);
                currentSubsection = entry.subsectionKey;
                currentIndex = -1;
            }

            // Handle index change (repeated subsection)
            if (entry.hasIndex && entry.index != currentIndex) {
                currentIndex = entry.index;
                // Repeat subsection header for each new index group
                if (entry.subsectionKey != null && !entry.subsectionKey.isEmpty()) {
                    Label subLabel = new Label(entry.subsectionKey);
                    subLabel.setStyle("-fx-font-size: " + bodyFont + "px; -fx-font-weight: bold; -fx-text-fill: #555; -fx-padding: " + uiScale.px(4) + "px 0 " + uiScale.px(2) + "px " + uiScale.px(12) + "px;");
                    contentBox.getChildren().add(subLabel);
                }
            }

            // Render label: value row
            HBox row = new HBox(uiScale.px(6));
            row.setPadding(new Insets(uiScale.px(1), 0, uiScale.px(1), uiScale.px(24)));

            Label keyLabel = new Label(entry.labelKey + ":");
            keyLabel.setStyle("-fx-font-size: " + bodyFont + "px; -fx-font-weight: bold; -fx-text-fill: #666; -fx-min-width: " + uiScale.px(100) + "px;");

            Label valueLabel = new Label(entry.value);
            valueLabel.setStyle("-fx-font-size: " + bodyFont + "px; -fx-text-fill: #333;");
            valueLabel.setWrapText(true);
            HBox.setHgrow(valueLabel, Priority.ALWAYS);

            row.getChildren().addAll(keyLabel, valueLabel);
            contentBox.getChildren().add(row);
        }
    }

    /**
     * Open the dialog with a fade-in animation.
     */
    public void open() {
        expanded = true;
        setVisible(true);

        updateLayout();

        FadeTransition fade = new FadeTransition(Duration.millis(200), this);
        fade.setFromValue(0.0);
        fade.setToValue(1.0);

        TranslateTransition slide = new TranslateTransition(Duration.millis(200), mainPanel);
        slide.setFromY(-20.0);
        slide.setToY(0.0);

        ParallelTransition pt = new ParallelTransition(fade, slide);
        pt.play();

        contentArea.requestFocus();
    }

    /**
     * Close the dialog with a fade-out animation.
     */
    public void close() {
        if (!expanded) return;
        expanded = false;

        FadeTransition fade = new FadeTransition(Duration.millis(150), this);
        fade.setFromValue(1.0);
        fade.setToValue(0.0);
        fade.setOnFinished(e -> {
            StackPane parent = (StackPane) getParent();
            if (parent != null) {
                parent.getChildren().remove(this);
            }
        });
        fade.play();
    }

    private void updateLayout() {
        double margin = uiScale.edgeMargin();
        double sceneWidth = getScene() != null ? getScene().getWidth() : uiScale.panelSmallMaxWidth();
        double sceneHeight = getScene() != null ? getScene().getHeight() : uiScale.px(600);
        if (sceneWidth < SMALL_SCREEN_THRESHOLD) {
            mainPanel.setMaxWidth(Double.MAX_VALUE);
            mainPanel.setMaxHeight(Double.MAX_VALUE);
            StackPane.setMargin(mainPanel, Insets.EMPTY);
            StackPane.setAlignment(mainPanel, Pos.CENTER);
            mainPanel.setStyle("-fx-background-color: white; -fx-padding: 0;");
        } else {
            mainPanel.setMaxWidth(uiScale.panelMaxWidth());
            mainPanel.setMaxHeight(Math.min(uiScale.px(600), sceneHeight - uiScale.px(80)));
            StackPane.setMargin(mainPanel, new Insets(margin));
            StackPane.setAlignment(mainPanel, Pos.CENTER);
            mainPanel.getStyleClass().add("search-overlay-panel");
        }
    }
}
