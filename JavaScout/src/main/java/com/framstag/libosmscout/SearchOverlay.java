package com.framstag.libosmscout;

import com.framstag.libosmscout.client.FavoriteLocation;
import com.framstag.libosmscout.client.FavoriteLocationGroup;
import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;

import javafx.animation.FadeTransition;
import javafx.animation.ParallelTransition;
import javafx.animation.TranslateTransition;
import javafx.application.Platform;
import javafx.concurrent.Task;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.input.MouseEvent;
import javafx.scene.shape.SVGPath;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.util.Duration;

import java.util.*;
import java.util.function.Consumer;
import java.util.stream.Collectors;

/**
 * Map overlay search control for JavaScout.
 * <p>
 * Floating search button in bottom-right corner that expands into
 * a search text field with detailed result list.
 * Search starts automatically while typing with debounce.
 * Includes a "Favorites" mode for browsing saved favorite locations.
 */
public class SearchOverlay extends StackPane {

    private static final double SMALL_SCREEN_THRESHOLD = 600.0;
    private static final int DEFAULT_LIMIT = 50;
    private static final int DEBOUNCE_MS = 400;

    private final OSMScoutClient client;
    private final UIScale uiScale;
    private final OverlayLayout overlayLayout;
    private final Runnable onNavigate;
    private final Button searchButton;

    // UI components
    private final VBox searchPanel;
    private final HBox titleBar;
    private final TextField searchField;
    private final Button cancelButton;
    private final ListView<LocationEntry> resultList;
    private final HBox searchBar;

    // Favorites mode
    private final TreeView<Object> favTree;
    private final VBox favContainer;
    private boolean favoritesLoaded = false;
    private java.util.Map<String, FavoriteLocationGroup> loadedFavGroups = new HashMap<>();

    private boolean expanded = false;
    private double mapCenterLat;
    private double mapCenterLon;
    private int targetMagnification = 16;
    private String lastSearchText = "";

    // Debounce
    private javafx.animation.PauseTransition debounceTimer;

    /** True while collapse() is clearing the result list; ignores selection change callbacks. */
    private boolean clearingSelection = false;

    /** True when collapse is triggered by a successful navigation; keeps the search marker. */
    private boolean keepSearchMarkerOnCollapse = false;

    /** Callback for route location picking mode. */
    private Consumer<LocationEntry> pickCallback;

    /** Callback invoked when the selected search result changes. */
    private final Consumer<LocationEntry> onSelectionChanged;

    /** Click-outside handler — added to scene when expanded. */
    private final EventHandler<MouseEvent> outsideHandler = this::onOutsideClick;

    private void onOutsideClick(MouseEvent e) {
        if (!expanded) return;
        double x = e.getSceneX();
        double y = e.getSceneY();
        var bounds = searchPanel.localToScene(searchPanel.getBoundsInLocal());
        if (!bounds.contains(x, y)) {
            collapse();
        }
    }

    /**
     * Create a search overlay.
     *
     * @param client     the OSMScoutClient for location search
     * @param uiScale    the DPI-aware UI scale helper
     * @param onNavigate callback invoked when user clicks a result
     * @param onSelectionChanged callback invoked when the highlighted result changes
     * @param searchButton the collapsed search button to hide/show
     */
    public SearchOverlay(OSMScoutClient client, UIScale uiScale, Runnable onNavigate,
                         Consumer<LocationEntry> onSelectionChanged,
                         Button searchButton) {
        this.client = client;
        this.uiScale = uiScale;
        this.overlayLayout = new OverlayLayout(uiScale);
        this.onNavigate = onNavigate;
        this.onSelectionChanged = onSelectionChanged;
        this.searchButton = searchButton;

        setPickOnBounds(false);
        setMouseTransparent(true);

        double thumb = uiScale.thumbSize();
        double controlHeight = uiScale.controlHeight();

        // --- Search panel (expanded state) ---
        searchPanel = new VBox(uiScale.px(0));
        searchPanel.setVisible(false);
        searchPanel.setOpacity(0.0);
        searchPanel.getStyleClass().add("search-overlay-panel");
        searchPanel.setMaxWidth(uiScale.panelMaxWidth());

        // Title bar
        titleBar = new HBox(uiScale.px(4));
        titleBar.setAlignment(Pos.CENTER_LEFT);
        titleBar.setPadding(uiScale.insets(8));
        titleBar.setStyle("-fx-background-color: #f0f0f0; -fx-background-radius: 4px 4px 0 0; -fx-border-color: #ddd; -fx-border-width: 0 0 1px 0;");

        Label titleLabel = new Label("Search location");
        titleLabel.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-font-weight: bold;");
        titleLabel.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(titleLabel, Priority.ALWAYS);

        Button closeButton = new Button("\u2715");
        closeButton.setStyle("-fx-background-color: transparent; -fx-text-fill: #999; -fx-font-size: " + uiScale.baseFontSize() + "px; -fx-cursor: hand;");
        closeButton.setMinSize(controlHeight, controlHeight);
        closeButton.setPrefSize(controlHeight, controlHeight);
        closeButton.setMaxSize(controlHeight, controlHeight);
        closeButton.setOnAction(e -> collapse());

        titleBar.getChildren().addAll(titleLabel, closeButton);

        // Search bar: text field + clear button + cancel button
        searchBar = new HBox(uiScale.px(4));
        searchBar.setAlignment(Pos.CENTER_LEFT);
        searchBar.setPadding(new Insets(uiScale.px(8), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        searchField = new TextField();
        searchField.setPromptText("Search location...");
        searchField.setMinHeight(controlHeight);
        searchField.setPrefHeight(controlHeight);
        searchField.setMaxHeight(controlHeight);
        searchField.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        HBox.setHgrow(searchField, Priority.ALWAYS);

        Button clearButton = new Button("\u2715");
        clearButton.getStyleClass().add("search-clear-button");
        clearButton.setMinSize(controlHeight, controlHeight);
        clearButton.setPrefSize(controlHeight, controlHeight);
        clearButton.setMaxSize(controlHeight, controlHeight);
        clearButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        clearButton.setOnAction(e -> {
            searchField.clear();
            searchField.requestFocus();
        });

        cancelButton = new Button("Cancel");
        cancelButton.getStyleClass().add("search-clear-button");
        cancelButton.setMinHeight(controlHeight);
        cancelButton.setPrefHeight(controlHeight);
        cancelButton.setMaxHeight(controlHeight);
        cancelButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        cancelButton.setOnAction(e -> {
            client.cancelSearch();
            collapse();
        });

        searchBar.getChildren().addAll(searchField, clearButton, cancelButton);

        // Result list
        resultList = new ListView<>();
        resultList.setPrefHeight(uiScale.px(300));
        resultList.setMaxHeight(uiScale.px(450));
        VBox.setMargin(resultList, new Insets(0, uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        resultList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(LocationEntry item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else {
                    VBox box = new VBox(uiScale.px(2));
                    box.setPadding(new Insets(uiScale.px(4), uiScale.px(6), uiScale.px(4), uiScale.px(6)));

                    // Line 1: match quality + label + address + postal area + region,
                    // with the distance to the map center right-aligned
                    String line1 = buildLine1(item);
                    HBox line1Box = new HBox(uiScale.px(6));
                    line1Box.setAlignment(Pos.CENTER_LEFT);

                    Label label1 = new Label(line1);
                    label1.getStyleClass().add("search-result-line1");
                    label1.setMaxWidth(Double.MAX_VALUE);
                    HBox.setHgrow(label1, Priority.ALWAYS);

                    double distanceMeters = LocationSearchRanker.haversine(
                            mapCenterLat, mapCenterLon, item.lat, item.lon);
                    Label distanceLabel = new Label(LocationSearchRanker.formatDistanceKm(distanceMeters));
                    distanceLabel.getStyleClass().add("search-result-distance");

                    line1Box.getChildren().addAll(label1, distanceLabel);

                    // Line 2: admin region hierarchy
                    String line2 = "";
                    if (item.adminRegionHierarchy != null && !item.adminRegionHierarchy.isEmpty()) {
                        line2 = "   \u2192 " + item.adminRegionHierarchy;
                    }
                    Label label2 = new Label(line2);
                    label2.getStyleClass().add("search-result-line2");

                    // Line 3: object type + offset
                    String line3 = "";
                    if (item.objectTypeName != null && !item.objectTypeName.isEmpty()) {
                        String refType = item.type != null ? item.type : "object";
                        line3 = "   - " + refType + " " + item.objectFileOffset + " " + item.objectTypeName;
                    }
                    Label label3 = new Label(line3);
                    label3.getStyleClass().add("search-result-line3");

                    box.getChildren().addAll(line1Box, label2, label3);
                    setGraphic(box);
                }
            }
        });

        resultList.setOnMouseClicked(e -> {
            LocationEntry selected = resultList.getSelectionModel().getSelectedItem();
            if (selected != null) {
                navigateTo(selected);
            }
        });

        resultList.getSelectionModel().selectedItemProperty().addListener(
            (obs, oldVal, newVal) -> {
                if (clearingSelection) {
                    return;
                }
                if (onSelectionChanged != null) {
                    onSelectionChanged.accept(newVal);
                }
            });

        resultList.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ENTER) {
                LocationEntry selected = resultList.getSelectionModel().getSelectedItem();
                if (selected != null) {
                    navigateTo(selected);
                }
            } else if (e.getCode() == KeyCode.ESCAPE) {
                collapse();
            }
        });

        searchPanel.getChildren().addAll(titleBar, searchBar, resultList);

        // Favorites container (hidden by default)
        favContainer = new VBox(uiScale.px(4));
        favContainer.setVisible(false);
        favContainer.setManaged(false);
        favContainer.setPadding(new Insets(uiScale.px(8), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        favContainer.setFillWidth(true);

        favTree = new TreeView<>();
        favTree.setPrefHeight(uiScale.px(300));
        favTree.setMaxHeight(uiScale.px(450));
        VBox.setVgrow(favTree, Priority.ALWAYS);
        VBox.setMargin(favTree, new Insets(0, 0, uiScale.px(8), 0));

        favTree.setCellFactory(tv -> new TreeCell<>() {
            @Override
            protected void updateItem(Object item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else if (item instanceof FavoriteLocationGroup group) {
                    setText(group.name + " (" + group.favorites.size() + ")");
                } else if (item instanceof FavoriteLocation fav) {
                    setText(fav.name + "  (" + String.format("%.4f", fav.lat) + ", " + String.format("%.4f", fav.lon) + ")");
                }
            }
        });

        favTree.setOnMouseClicked(e -> {
            TreeItem<Object> selected = favTree.getSelectionModel().getSelectedItem();
            if (selected != null && selected.getValue() instanceof FavoriteLocation fav) {
                navigateToFavorite(fav);
            }
        });

        favTree.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ENTER) {
                TreeItem<Object> selected = favTree.getSelectionModel().getSelectedItem();
                if (selected != null && selected.getValue() instanceof FavoriteLocation fav) {
                    navigateToFavorite(fav);
                }
            } else if (e.getCode() == KeyCode.ESCAPE) {
                collapse();
            }
        });

        Label favHint = new Label("Select a favorite to navigate.");
        favHint.setStyle("-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-text-fill: #666;");

        favContainer.getChildren().addAll(favTree, favHint);
        searchPanel.getChildren().add(favContainer);

        // Escape closes search, arrow keys move focus to result list
        searchField.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ESCAPE) {
                collapse();
            } else if (e.getCode() == KeyCode.DOWN) {
                resultList.requestFocus();
                if (!resultList.getItems().isEmpty()) {
                    resultList.getSelectionModel().select(0);
                }
            } else if (e.getCode() == KeyCode.UP) {
                resultList.requestFocus();
                if (!resultList.getItems().isEmpty()) {
                    resultList.getSelectionModel().select(resultList.getItems().size() - 1);
                }
            }
        });

        // Debounced search on text change
        debounceTimer = new javafx.animation.PauseTransition(Duration.millis(DEBOUNCE_MS));
        debounceTimer.setOnFinished(e -> performSearch());

        searchField.textProperty().addListener((obs, oldVal, newVal) -> {
            if (expanded) {
                debounceTimer.playFromStart();
            }
        });

        // Also search on Enter immediately
        searchField.setOnAction(e -> {
            debounceTimer.stop();
            performSearch();
        });

        // Add to layout
        getChildren().add(searchPanel);

        // Position: bottom-right
        setAlignment(searchPanel, Pos.BOTTOM_RIGHT);
    }

    private void updatePanelAlignment() {
        double windowWidth = getScene() != null ? getScene().getWidth() : uiScale.panelSmallMaxWidth();
        overlayLayout.anchorPanel(this, searchPanel, windowWidth);
    }

    private String buildLine1(LocationEntry entry) {
        StringBuilder sb = new StringBuilder();

        // Match quality symbol
        boolean isMatch = "match".equals(entry.matchQuality);
        sb.append(isMatch ? "= " : "~ ");

        // Components
        boolean first = true;
        if (entry.label != null && !entry.label.isEmpty()) {
            sb.append(entry.label);
            first = false;
        }
        if (entry.postalArea != null && !entry.postalArea.isEmpty()) {
            if (!first) sb.append(" ");
            sb.append("~ PostalArea (").append(entry.postalArea).append(")");
            first = false;
        }
        if (entry.region != null && entry.region.length > 0) {
            if (!first) sb.append(" ");
            sb.append("= Region (").append(entry.region[0]).append(")");
        }
        return sb.toString();
    }

    /**
     * Update the map center used for distance-based sorting.
     */
    public void setMapCenter(double lat, double lon) {
        this.mapCenterLat = lat;
        this.mapCenterLon = lon;
    }

    /**
     * Get the current map center latitude (updated on navigation).
     */
    public double getMapCenterLat() {
        return mapCenterLat;
    }

    /**
     * Get the current map center longitude (updated on navigation).
     */
    public double getMapCenterLon() {
        return mapCenterLon;
    }

    /**
     * Get the target magnification for search result navigation.
     */
    public int getTargetMagnification() {
        return targetMagnification;
    }

    public void toggleExpand() {
        if (expanded) {
            collapse();
        } else {
            expand();
        }
    }

    /**
     * Programmatically expand the search overlay (e.g. from keyboard shortcut).
     */
    public void openSearch() {
        if (!expanded) {
            expand();
            // Restore last search text
            if (!lastSearchText.isEmpty()) {
                searchField.setText(lastSearchText);
                searchField.selectAll();
            }
        }
        searchField.requestFocus();
    }

    /**
     * Open search in "pick" mode for route location selection.
     * When user selects a result, the callback is invoked with the chosen location
     * and the search overlay returns to normal mode.
     *
     * @param prompt       placeholder text for the search field
     * @param initialQuery optional initial query to pre-fill
     * @param callback     invoked with the selected LocationEntry
     */
    public void pickForRoute(String prompt, String initialQuery, Consumer<LocationEntry> callback) {
        this.pickCallback = callback;
        searchField.setPromptText(prompt);
        expand();
        if (initialQuery != null && !initialQuery.isEmpty() && !looksLikeCoordinate(initialQuery)) {
            searchField.setText(initialQuery);
            searchField.selectAll();
            debounceTimer.playFromStart();
        }
    }

    private boolean looksLikeCoordinate(String text) {
        // Coordinate labels contain a comma between two numbers
        return text.matches(".*\\d+\\s*,\\s*[-]?\\d+.*");
    }

    /**
     * Get the search panel bounds in scene coordinates (for click-outside checks).
     */
    public javafx.geometry.Bounds getSearchPanelBoundsInScene() {
        if (searchPanel.isVisible()) {
            return searchPanel.localToScene(searchPanel.getBoundsInLocal());
        }
        return null;
    }

    private void expand() {
        expanded = true;
        searchPanel.setVisible(true);
        if (searchButton != null) {
            searchButton.setVisible(false);
        }
        updatePanelAlignment();
        setMouseTransparent(false);

        // Close on click outside — listen on scene
        Platform.runLater(() -> {
            var scene = getScene();
            if (scene != null) {
                scene.addEventFilter(MouseEvent.MOUSE_PRESSED, outsideHandler);
            }
        });

        FadeTransition fade = new FadeTransition(Duration.millis(200), searchPanel);
        fade.setFromValue(0.0);
        fade.setToValue(1.0);

        TranslateTransition slide = new TranslateTransition(Duration.millis(200), searchPanel);
        slide.setFromY(20.0);
        slide.setToY(0.0);

        ParallelTransition pt = new ParallelTransition(fade, slide);
        pt.play();

        searchField.requestFocus();
    }

    private void collapse() {
        // Remember last non-empty search text
        String text = searchField.getText();
        if (!text.trim().isEmpty()) {
            lastSearchText = text;
        }
        expanded = false;
        searchField.clear();
        clearingSelection = true;
        try {
            resultList.getItems().clear();
        } finally {
            clearingSelection = false;
        }
        debounceTimer.stop();
        setMouseTransparent(true);
        if (searchButton != null) {
            searchButton.setVisible(true);
        }

        // Clear the search-selection marker if the overlay was closed without
        // navigating to a result (Esc, Cancel, close button, click outside).
        if (!keepSearchMarkerOnCollapse && onSelectionChanged != null) {
            onSelectionChanged.accept(null);
        }
        keepSearchMarkerOnCollapse = false;

        // Remove scene listener
        var scene = getScene();
        if (scene != null) {
            scene.removeEventFilter(MouseEvent.MOUSE_PRESSED, outsideHandler);
        }

        FadeTransition fade = new FadeTransition(Duration.millis(150), searchPanel);
        fade.setFromValue(1.0);
        fade.setToValue(0.0);
        fade.setOnFinished(e -> searchPanel.setVisible(false));
        fade.play();
    }

    private void performSearch() {
        String query = searchField.getText().trim();
        if (query.isEmpty()) {
            resultList.getItems().clear();
            return;
        }

        Task<List<LocationEntry>> searchTask = new Task<>() {
            @Override
            protected List<LocationEntry> call() {
                // Scope the search to the current map region (OSMScout2 parity);
                // falls back to the whole database when no region is resolvable.
                String defaultRegion = client.getRegion(mapCenterLat, mapCenterLon);
                LocationEntry[] results = client.searchLocations(query, DEFAULT_LIMIT, defaultRegion, false);
                if (results == null) {
                    return List.of();
                }
                List<LocationEntry> sorted = Arrays.stream(results)
                        .sorted(LocationSearchRanker.comparator(query, mapCenterLat, mapCenterLon))
                        .collect(Collectors.toList());
                return LocationSearchRanker.deduplicate(sorted, mapCenterLat, mapCenterLon);
            }
        };

        searchTask.setOnSucceeded(e -> {
            // A new query replaces any previous search-selection marker
            if (onSelectionChanged != null) {
                onSelectionChanged.accept(null);
            }
            List<LocationEntry> sorted = searchTask.getValue();
            resultList.getItems().setAll(sorted);
            if (!sorted.isEmpty()) {
                resultList.getSelectionModel().select(0);
            }
            updateLayout();
            updatePanelAlignment();
        });

        searchTask.setOnFailed(e -> {
            Throwable err = searchTask.getException();
            Log.error("Search failed: " +
                    (err != null ? err.getMessage() : "unknown"));
        });

        Thread thread = new Thread(searchTask, "location-search");
        thread.setDaemon(true);
        thread.start();
    }

    private void navigateTo(LocationEntry entry) {
        navigateTo(entry, true);
    }

    private void navigateTo(LocationEntry entry, boolean markAsSearchSelection) {
        if (pickCallback != null) {
            // Route pick mode — return location without navigating
            Consumer<LocationEntry> cb = pickCallback;
            pickCallback = null;
            searchField.setPromptText("Search location...");
            collapse();
            cb.accept(entry);
            return;
        }

        // Keep the search-selection marker at the navigated result so it remains
        // visible after the overlay closes. Favorites use the same path but should
        // not set the search marker.
        if (markAsSearchSelection && onSelectionChanged != null) {
            onSelectionChanged.accept(entry);
        }

        mapCenterLat = entry.lat;
        mapCenterLon = entry.lon;
        targetMagnification = 16;
        keepSearchMarkerOnCollapse = true;
        collapse();
        if (onNavigate != null) {
            onNavigate.run();
        }
    }

    /**
     * Switch between OSM search mode and favorites mode.
     */
    private void setFavoritesMode(boolean enabled) {
        searchBar.setVisible(!enabled);
        searchBar.setManaged(!enabled);
        resultList.setVisible(!enabled);
        resultList.setManaged(!enabled);
        favContainer.setVisible(enabled);
        favContainer.setManaged(enabled);

        if (enabled) {
            loadFavorites();
            buildFavoritesTree();
        }
    }

    /**
     * Load favorite groups from the client into memory.
     */
    private void loadFavorites() {
        FavoriteLocationGroup[] groups = client.getFavoriteGroups();
        loadedFavGroups.clear();
        if (groups != null) {
            for (FavoriteLocationGroup g : groups) {
                loadedFavGroups.put(g.name, g);
            }
        }
        favoritesLoaded = true;
    }

    /**
     * Build the favorites tree view from loaded groups.
     */
    private void buildFavoritesTree() {
        TreeItem<Object> root = new TreeItem<>("Favorites");
        root.setExpanded(true);

        for (FavoriteLocationGroup group : loadedFavGroups.values()) {
            TreeItem<Object> groupItem = new TreeItem<>(group);
            groupItem.setExpanded(true);
            for (FavoriteLocation fav : group.favorites) {
                groupItem.getChildren().add(new TreeItem<>(fav));
            }
            root.getChildren().add(groupItem);
        }

        favTree.setRoot(root);
        favTree.setShowRoot(false);
    }

    /**
     * Reload favorite groups from the client and refresh the favorites tree.
     */
    public void refreshFavorites() {
        loadFavorites();
        if (favContainer.isVisible()) {
            buildFavoritesTree();
        }
    }

    /**
     * Navigate to a selected favorite location.
     */
    private void navigateToFavorite(FavoriteLocation fav) {
        LocationEntry entry = new LocationEntry();
        entry.label = fav.name;
        entry.lat = fav.lat;
        entry.lon = fav.lon;
        entry.matchQuality = "favorite";
        navigateTo(entry, false);
    }

    private void updateLayout() {
        double windowWidth = getScene() != null ? getScene().getWidth() : uiScale.panelSmallMaxWidth();
        if (windowWidth < uiScale.smallScreenThreshold()) {
            searchPanel.setMaxWidth(Double.MAX_VALUE);
            searchField.setPrefWidth(Region.USE_COMPUTED_SIZE);
        } else {
            searchPanel.setMaxWidth(uiScale.panelMaxWidth());
            searchField.setPrefWidth(uiScale.px(350));
        }
    }
}

