package com.framstag.libosmscout;

import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.ObjectDescription;
import com.framstag.libosmscout.client.PoiCategories;
import com.framstag.libosmscout.client.PoiEntry;

import javafx.animation.FadeTransition;
import javafx.animation.PauseTransition;
import javafx.concurrent.Task;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.ListCell;
import javafx.scene.control.ListView;
import javafx.scene.control.Slider;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.util.Duration;

import java.util.Arrays;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.DoubleSupplier;

/**
 * POI search overlay: choose a POI category, define the search radius with a
 * stepped slider, search around the current map center, and browse results.
 * <p>
 * Clicking a result pans the map to the POI. Long-pressing a result shows the
 * details dialog (reusing {@link DescriptionOverlay} via the
 * {@code onShowDescription} callback).
 */
public class PoiSearchOverlay extends StackPane {

    /** Maximum number of results per search. */
    private static final int MAX_RESULTS = 100;

    /** Slider steps map to these radii (meters). */
    private static final double[] RADIUS_STEPS_M = {500, 1000, 2000, 5000, 10000, 20000};

    /** Category display names mapped to {@link PoiCategories} ids. */
    private static final String[][] CATEGORIES = {
            {"Hotels", PoiCategories.HOTELS},
            {"Restaurants", PoiCategories.RESTAURANTS},
            {"Grocery store", PoiCategories.GROCERY},
            {"Viewpoint", PoiCategories.VIEWPOINT},
            {"Museum", PoiCategories.MUSEUM},
            {"Gas station", PoiCategories.FUEL},
            {"Charging station", PoiCategories.CHARGING_STATION},
            {"ATM", PoiCategories.ATM},
            {"Tourism", PoiCategories.TOURISM},
            {"Parking", PoiCategories.PARKING},
            {"Police station", PoiCategories.POLICE},
            {"Hospital", PoiCategories.HOSPITAL},
            {"Doctors office", PoiCategories.DOCTORS},
            {"Public transport", PoiCategories.PUBLIC_TRANSPORT},
    };

    /** Long-press timeout on result entries (same default as map long-press). */
    private static final long LONG_PRESS_TIMEOUT_MS = 500;

    private final OSMScoutClient client;
    private final UIScale uiScale;
    private final OverlayLayout overlayLayout;

    private final DoubleSupplier mapCenterLat;
    private final DoubleSupplier mapCenterLon;
    private final Consumer<PoiEntry> onNavigate;
    private final Consumer<ObjectDescription> onShowDescription;

    private final VBox searchPanel;
    private final ComboBox<String> categoryCombo;
    private final Slider radiusSlider;
    private final Label radiusValueLabel;

    private final ListView<PoiEntry> resultList;

    private final PauseTransition longPressTimer = new PauseTransition(Duration.millis(LONG_PRESS_TIMEOUT_MS));
    private boolean longPressTriggered;
    private PoiEntry longPressEntry;

    private boolean open;

    /**
     * @param client            the OSMScoutClient
     * @param uiScale           DPI-aware scaling
     * @param mapCenterLat      current map center latitude (evaluated at search time)
     * @param mapCenterLon      current map center longitude (evaluated at search time)
     * @param onNavigate        invoked when the user clicks a result (pan map to POI)
     * @param onShowDescription invoked with the POI's {@link ObjectDescription};
     *                          an empty description means "no description available"
     */
    public PoiSearchOverlay(OSMScoutClient client,
                            UIScale uiScale,
                            DoubleSupplier mapCenterLat,
                            DoubleSupplier mapCenterLon,
                            Consumer<PoiEntry> onNavigate,
                            Consumer<ObjectDescription> onShowDescription) {
        this.client = client;
        this.uiScale = uiScale;
        this.overlayLayout = new OverlayLayout(uiScale);
        this.mapCenterLat = mapCenterLat;
        this.mapCenterLon = mapCenterLon;
        this.onNavigate = onNavigate;
        this.onShowDescription = onShowDescription;

        setPickOnBounds(false);
        setMouseTransparent(true);

        double thumb = uiScale.thumbSize();
        double controlHeight = uiScale.controlHeight();

        searchPanel = new VBox(uiScale.px(0));
        searchPanel.setVisible(false);
        searchPanel.setOpacity(0.0);
        searchPanel.getStyleClass().add("search-overlay-panel");
        searchPanel.setMaxWidth(uiScale.panelMaxWidth());

        // --- Title bar ---
        HBox titleBar = new HBox(uiScale.px(4));
        titleBar.setAlignment(Pos.CENTER_LEFT);
        titleBar.setPadding(uiScale.insets(8));
        titleBar.setStyle("-fx-background-color: #f0f0f0; -fx-background-radius: 4px 4px 0 0; -fx-border-color: #ddd; -fx-border-width: 0 0 1px 0;");

        Label titleLabel = new Label("Search POIs");
        titleLabel.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-font-weight: bold;");
        titleLabel.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(titleLabel, Priority.ALWAYS);

        Button closeButton = new Button("\u2715");
        closeButton.setStyle("-fx-background-color: transparent; -fx-text-fill: #999; -fx-font-size: " + uiScale.baseFontSize() + "px; -fx-cursor: hand;");
        closeButton.setMinSize(controlHeight, controlHeight);
        closeButton.setPrefSize(controlHeight, controlHeight);
        closeButton.setMaxSize(controlHeight, controlHeight);
        closeButton.setOnAction(e -> close());

        titleBar.getChildren().addAll(titleLabel, closeButton);

        // --- Category selection ---
        Label categoryLabel = new Label("Category");
        categoryLabel.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");

        categoryCombo = new ComboBox<>();
        for (String[] category : CATEGORIES) {
            categoryCombo.getItems().add(category[0]);
        }
        categoryCombo.getSelectionModel().select(0);
        categoryCombo.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(categoryCombo, Priority.ALWAYS);
        categoryCombo.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");

        HBox categoryRow = new HBox(uiScale.px(8));
        categoryRow.setAlignment(Pos.CENTER_LEFT);
        categoryRow.setPadding(new Insets(uiScale.px(8), uiScale.px(10), uiScale.px(2), uiScale.px(10)));
        categoryRow.getChildren().addAll(categoryLabel, categoryCombo);

        // --- Search area slider ---
        Label radiusLabel = new Label("Search area");
        radiusLabel.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");

        radiusValueLabel = new Label();
        radiusValueLabel.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-font-weight: bold;");
        radiusValueLabel.setMinWidth(uiScale.px(56));

        radiusSlider = new Slider(0, RADIUS_STEPS_M.length - 1, 3);
        radiusSlider.setBlockIncrement(1.0);
        radiusSlider.setMajorTickUnit(1.0);
        radiusSlider.setMinorTickCount(0);
        radiusSlider.setSnapToTicks(true);
        radiusSlider.setShowTickMarks(true);
        radiusSlider.setMaxWidth(Double.MAX_VALUE);
        radiusSlider.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        HBox.setHgrow(radiusSlider, Priority.ALWAYS);
        radiusSlider.valueProperty().addListener((obs, oldVal, newVal) -> updateRadiusLabel());
        updateRadiusLabel();

        HBox radiusRow = new HBox(uiScale.px(8));
        radiusRow.setAlignment(Pos.CENTER_LEFT);
        radiusRow.setPadding(new Insets(uiScale.px(4), uiScale.px(10), uiScale.px(2), uiScale.px(10)));
        radiusRow.getChildren().addAll(radiusLabel, radiusSlider, radiusValueLabel);

        // --- Search trigger ---
        Button searchButton = new Button("Search");
        searchButton.getStyleClass().add("search-trigger-button");
        searchButton.setMinHeight(controlHeight);
        searchButton.setPrefHeight(controlHeight);
        searchButton.setMaxHeight(controlHeight);
        searchButton.setMaxWidth(Double.MAX_VALUE);
        searchButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-cursor: hand;");
        HBox.setHgrow(searchButton, Priority.ALWAYS);
        searchButton.setOnAction(e -> performSearch());
        HBox searchButtonRow = new HBox(uiScale.px(8));
        searchButtonRow.setPadding(new Insets(uiScale.px(6), uiScale.px(10), uiScale.px(2), uiScale.px(10)));
        searchButtonRow.getChildren().add(searchButton);

        // --- Result list ---
        resultList = new ListView<>();
        resultList.setPrefHeight(uiScale.px(300));
        resultList.setMaxHeight(uiScale.px(450));
        VBox.setMargin(resultList, new Insets(0, uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        resultList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(PoiEntry item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else {
                    VBox box = new VBox(uiScale.px(2));
                    box.setPadding(new Insets(uiScale.px(4), uiScale.px(6), uiScale.px(4), uiScale.px(6)));

                    String label = (item.label != null && !item.label.isEmpty())
                            ? item.label
                            : "(unnamed)";
                    Label line1 = new Label(label);
                    line1.getStyleClass().add("search-result-line1");

                    String objectType = item.objectType != null ? item.objectType : "poi";
                    Label line2 = new Label("   " + objectType + "  \u00b7  " + formatDistance(item.distance));
                    line2.getStyleClass().add("search-result-line2");

                    box.getChildren().addAll(line1, line2);
                    setGraphic(box);
                }
            }
        });

        // Long-press on a result entry shows the details dialog; a plain click
        // navigates the map. The timer suppresses the click when it fires.
        resultList.addEventFilter(MouseEvent.MOUSE_PRESSED, this::onResultMousePressed);
        resultList.addEventFilter(MouseEvent.MOUSE_RELEASED, this::onResultMouseReleased);
        resultList.addEventFilter(MouseEvent.MOUSE_CLICKED, this::onResultMouseClicked);

        longPressTimer.setOnFinished(e -> onLongPressFired());

        searchPanel.getChildren().addAll(titleBar, categoryRow, radiusRow, searchButtonRow, resultList);
        getChildren().add(searchPanel);
    }

    private void updateRadiusLabel() {
        int step = (int) Math.round(radiusSlider.getValue());
        step = Math.max(0, Math.min(step, RADIUS_STEPS_M.length - 1));
        double meters = RADIUS_STEPS_M[step];
        radiusValueLabel.setText(formatDistance(meters));
    }

    private String formatDistance(double meters) {
        if (meters >= 1000.0) {
            return String.format("%.1f km", meters / 1000.0);
        }
        return String.format("%.0f m", meters);
    }

    private int selectedRadiusStep() {
        int step = (int) Math.round(radiusSlider.getValue());
        return Math.max(0, Math.min(step, RADIUS_STEPS_M.length - 1));
    }

    private String selectedCategory() {
        int index = categoryCombo.getSelectionModel().getSelectedIndex();
        if (index < 0 || index >= CATEGORIES.length) {
            return null;
        }
        return CATEGORIES[index][1];
    }

    /**
     * Open the overlay and trigger a search with the current selection.
     */
    public void open() {
        if (open) {
            return;
        }
        open = true;
        setMouseTransparent(false);
        searchPanel.setVisible(true);
        searchPanel.setOpacity(1.0);
        updatePanelAlignment();
        performSearch();
    }

    /**
     * Close the overlay.
     */
    public void close() {
        if (!open) {
            return;
        }
        open = false;
        longPressTimer.stop();
        longPressTriggered = false;
        longPressEntry = null;
        resultList.getItems().clear();
        setMouseTransparent(true);
        FadeTransition fade = new FadeTransition(Duration.millis(150), searchPanel);
        fade.setFromValue(1.0);
        fade.setToValue(0.0);
        fade.setOnFinished(e -> searchPanel.setVisible(false));
        fade.play();
    }

    private void updatePanelAlignment() {
        double windowWidth = getScene() != null ? getScene().getWidth() : uiScale.panelSmallMaxWidth();
        overlayLayout.anchorPanel(this, searchPanel, windowWidth);
    }

    private void performSearch() {
        String category = selectedCategory();
        if (category == null) {
            return;
        }
        double radiusMeters = RADIUS_STEPS_M[selectedRadiusStep()];
        double lat = mapCenterLat.getAsDouble();
        double lon = mapCenterLon.getAsDouble();

        Task<List<PoiEntry>> searchTask = new Task<>() {
            @Override
            protected List<PoiEntry> call() {
                PoiEntry[] results = client.searchPOIs(category, lat, lon, radiusMeters, MAX_RESULTS);
                if (results == null) {
                    return List.of();
                }
                return Arrays.asList(results);
            }
        };

        searchTask.setOnSucceeded(e -> {
            List<PoiEntry> entries = searchTask.getValue();
            resultList.getItems().setAll(entries);
            if (!entries.isEmpty()) {
                resultList.getSelectionModel().select(0);
            }
            updateLayout();
            updatePanelAlignment();
        });

        searchTask.setOnFailed(e -> {
            Throwable err = searchTask.getException();
            Log.error("[PoiSearchOverlay] POI search failed: " +
                    (err != null ? err.getMessage() : "unknown"));
        });

        Thread thread = new Thread(searchTask, "poi-search");
        thread.setDaemon(true);
        thread.start();
    }

    private void updateLayout() {
        // No-op placeholder for future responsive tweaks (mirrors SearchOverlay).
    }

    // ---- Result interactions ----

    private PoiEntry itemForEvent(MouseEvent event) {
        Node node = event.getPickResult().getIntersectedNode();
        while (node != null && !(node instanceof ListCell<?>)) {
            node = node.getParent();
        }
        if (node instanceof ListCell<?> cell && cell.getItem() instanceof PoiEntry entry) {
            return entry;
        }
        return null;
    }

    private void onResultMousePressed(MouseEvent event) {
        PoiEntry entry = itemForEvent(event);
        if (entry == null) {
            return;
        }
        longPressEntry = entry;
        longPressTriggered = false;
        longPressTimer.playFromStart();
    }

    private void onResultMouseReleased(MouseEvent event) {
        longPressTimer.stop();
    }

    private void onResultMouseClicked(MouseEvent event) {
        if (longPressTriggered) {
            longPressTriggered = false;
            longPressEntry = null;
            event.consume();
            return;
        }
        if (!event.isPrimaryButtonDown() && event.getButton() != javafx.scene.input.MouseButton.PRIMARY) {
            return;
        }
        PoiEntry entry = itemForEvent(event);
        if (entry != null) {
            onNavigate.accept(entry);
        }
    }

    private void onLongPressFired() {
        PoiEntry entry = longPressEntry;
        if (entry == null) {
            return;
        }
        longPressTriggered = true;

        Task<ObjectDescription> descTask = new Task<>() {
            @Override
            protected ObjectDescription call() {
                return client.getDescription(entry.lat, entry.lon, 18);
            }
        };

        descTask.setOnSucceeded(e -> {
            ObjectDescription desc = descTask.getValue();
            if (desc == null) {
                desc = new ObjectDescription(java.util.Collections.emptyList());
            }
            onShowDescription.accept(desc);
        });

        descTask.setOnFailed(e -> {
            Throwable err = descTask.getException();
            Log.error("[PoiSearchOverlay] Description lookup failed: " +
                    (err != null ? err.getMessage() : "unknown"));
        });

        Thread thread = new Thread(descTask, "poi-description-lookup");
        thread.setDaemon(true);
        thread.start();
    }
}
