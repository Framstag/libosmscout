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
import javafx.scene.control.ListCell;
import javafx.scene.control.ListView;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.util.Duration;

import java.util.List;
import java.util.function.Consumer;

/**
 * Overlay dialog listing candidate objects found at a long-pressed map
 * coordinate, letting the user choose which object's details to view.
 * <p>
 * Visually consistent with {@link DescriptionOverlay}: same panel styling,
 * fade/slide animation, fullscreen on small screens, click-outside or Escape
 * to close. Entries use the same description format as search results via
 * {@link SearchResultCell}.
 */
public class CandidatePickerOverlay extends StackPane {

    private static final double SMALL_SCREEN_THRESHOLD = 600.0;

    private final UIScale uiScale;
    private final VBox mainPanel;
    private final StackPane contentArea;
    private final Consumer<ObjectDescription> onSelect;

    private boolean expanded = false;

    /**
     * Create a candidate picker overlay.
     *
     * @param uiScale    the DPI-aware UI scale helper
     * @param candidates ranked candidate descriptions, in display order
     * @param onSelect   invoked with the chosen candidate; the picker closes first
     */
    public CandidatePickerOverlay(UIScale uiScale, List<ObjectDescription> candidates,
                                  Consumer<ObjectDescription> onSelect) {
        this.uiScale = uiScale;
        this.onSelect = onSelect;

        double controlHeight = uiScale.controlHeight();
        double baseFont = uiScale.baseFontSize();
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

        Label titleLabel = new Label("Select Object");
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

        // Candidate list
        ListView<ObjectDescription> resultList = new ListView<>();
        resultList.setPrefHeight(uiScale.px(300));
        resultList.setMaxHeight(uiScale.px(450));
        VBox.setVgrow(resultList, Priority.ALWAYS);
        resultList.getItems().setAll(candidates);

        resultList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(ObjectDescription item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else {
                    setGraphic(SearchResultCell.create(uiScale,
                        candidateName(item),
                        null,
                        candidateRefLine(item)));
                }
            }
        });

        resultList.setOnMouseClicked(e -> {
            ObjectDescription selected = resultList.getSelectionModel().getSelectedItem();
            if (selected != null) {
                select(selected);
            }
        });

        resultList.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ENTER) {
                ObjectDescription selected = resultList.getSelectionModel().getSelectedItem();
                if (selected != null) {
                    select(selected);
                }
            }
        });

        mainPanel.getChildren().addAll(titleBar, resultList);

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

    private void select(ObjectDescription candidate) {
        close();
        if (onSelect != null) {
            onSelect.accept(candidate);
        }
    }

    /**
     * Display name of a candidate, taken from the description entries
     * (Name / NameAlt / NameShort), falling back to the first non-type value.
     */
    private static String candidateName(ObjectDescription desc) {
        for (DescriptionEntry e : desc.getEntries()) {
            if (("Name".equals(e.labelKey) || "NameAlt".equals(e.labelKey) || "NameShort".equals(e.labelKey))
                && e.value != null && !e.value.isEmpty()) {
                return e.value;
            }
        }
        for (DescriptionEntry e : desc.getEntries()) {
            if (!"Type".equals(e.labelKey) && e.value != null && !e.value.isEmpty()) {
                return e.value;
            }
        }
        if (desc.getObjectTypeName() != null && !desc.getObjectTypeName().isEmpty()) {
            return desc.getObjectTypeName();
        }
        return "(unnamed)";
    }

    /**
     * Tertiary line mirroring the search result format:
     * "- &lt;refType&gt; &lt;fileOffset&gt; &lt;typeName&gt;".
     */
    private static String candidateRefLine(ObjectDescription desc) {
        String refType = desc.getObjectRefType() != null ? desc.getObjectRefType() : "object";
        String typeName = desc.getObjectTypeName() != null ? desc.getObjectTypeName() : "";
        return "   - " + refType + " " + desc.getObjectFileOffset() + " " + typeName;
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
