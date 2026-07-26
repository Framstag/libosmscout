package com.framstag.libosmscout;

import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.RouteCallback;
import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.RouteInstruction;
import com.framstag.libosmscout.client.RoutingProfile;
import com.framstag.libosmscout.client.Vehicle;

import javafx.animation.FadeTransition;
import javafx.animation.ParallelTransition;
import javafx.animation.TranslateTransition;
import javafx.application.Platform;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.control.ScrollPane;
import javafx.scene.input.KeyCode;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Pane;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.shape.SVGPath;
import javafx.util.Duration;

/**
 * Map overlay route control for JavaScout.
 * <p>
 * Floating route button in bottom-right corner (above search button) that expands into
 * a route panel. Start/destination are shown as read-only labels. Location selection
 * reuses the existing {@link SearchOverlay} via callbacks.
 * Turn-by-turn route description shown below the parameter box after calculation.
 */
public class RoutePanel extends StackPane {

    private final OSMScoutClient client;
    private final UIScale uiScale;
    private final Runnable onRouteReady;
    private final LocationPicker picker;
    private final FavoritePicker favPicker;
    private final Button routeButton;
    private final OverlayLayout overlayLayout;

    // Route state
    private LocationEntry startLocation;
    private LocationEntry destLocation;
    private RouteEntry currentRoute;
    private boolean routeActive = false;

    // Routing profile
    private RoutingProfile routingProfile = new RoutingProfile();

    // Vehicle selector buttons
    private final Button carButton;
    private final Button bicycleButton;
    private final Button pedestrianButton;
    private final javafx.scene.control.CheckBox avoidTollsCheck = new javafx.scene.control.CheckBox("Avoid tolls");
    private final javafx.scene.control.CheckBox avoidFerriesCheck = new javafx.scene.control.CheckBox("Avoid ferries");

    // UI components
    private final VBox routePanel;
    private final Label startLabel;
    private final Label destLabel;
    private final Button calculateButton;
    private final Button clearButton;
    private final VBox instructionBox;
    private final ScrollPane instructionScroll;
    private final VBox progressBox;
    private final Label progressLabel;
    private final ProgressIndicator progressIndicator;
    private final Button cancelButton;

    // Navigation status display
    private final Label navigationStatusLabel;
    private boolean navigationActive = false;

    // Reroute status
    private final Label rerouteStatusLabel;
    private javafx.animation.Timeline rerouteToastTimer;

    private boolean expanded = false;

    /** Scene change listener for route panel height. */
    private javafx.beans.value.ChangeListener<javafx.scene.Scene> routePanelHeightListener;

    /** Click-outside handler — added to scene when expanded. */
    private final EventHandler<MouseEvent> outsideHandler = this::onOutsideClick;

    private void onOutsideClick(MouseEvent e) {
        if (!expanded) return;
        double x = e.getSceneX();
        double y = e.getSceneY();
        var rpBounds = routePanel.localToScene(routePanel.getBoundsInLocal());
        if (rpBounds.contains(x, y)) return;
        // Don't collapse if click is inside search overlay
        var parent = getParent();
        if (parent instanceof javafx.scene.layout.StackPane) {
            for (var child : ((javafx.scene.layout.StackPane) parent).getChildren()) {
                if (child instanceof SearchOverlay && child.isVisible()) {
                    var soBounds = ((SearchOverlay) child).getSearchPanelBoundsInScene();
                    if (soBounds != null && soBounds.contains(x, y)) return;
                }
            }
        }
        collapse();
    }

    /** Callback interface for picking a location. */
    @FunctionalInterface
    public interface LocationPicker {
        void pickLocation(String prompt, String initialQuery,
                          java.util.function.Consumer<LocationEntry> callback);
    }

    /** Callback interface for picking a favorite location. */
    @FunctionalInterface
    public interface FavoritePicker {
        void pickFavorite(String prompt, java.util.function.Consumer<LocationEntry> callback);
    }

    /**
     * Create a route overlay panel.
     *
     * @param client       the OSMScoutClient for search and routing
     * @param uiScale      the DPI-aware UI scale helper
     * @param onRouteReady callback when route changes (ready or cleared)
     * @param picker       callback to open location search for picking start/dest
     * @param favPicker    callback to open favorites picker for picking start/dest
     */
    public RoutePanel(OSMScoutClient client, UIScale uiScale, Runnable onRouteReady,
                      LocationPicker picker, FavoritePicker favPicker,
                      Button routeButton) {
        this.client = client;
        this.uiScale = uiScale;
        this.onRouteReady = onRouteReady;
        this.picker = picker;
        this.favPicker = favPicker;
        this.routeButton = routeButton;
        this.overlayLayout = new OverlayLayout(uiScale);

        setPickOnBounds(false);
        setMouseTransparent(true);

        double thumb = uiScale.thumbSize();
        double controlHeight = uiScale.controlHeight();

        // --- Route panel (expanded state) ---
        routePanel = new VBox(uiScale.px(6));
        routePanel.setVisible(false);
        routePanel.setOpacity(0.0);
        routePanel.getStyleClass().add("route-overlay-panel");
        routePanel.setMaxWidth(uiScale.panelMaxWidth());
        routePanel.setStyle("-fx-background-color: rgba(255,255,255,0.95); -fx-background-radius: 6px; -fx-padding: 0px; -fx-effect: dropshadow(gaussian, rgba(0,0,0,0.3), 8, 0, 0, 4);");
        routePanel.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ESCAPE) {
                collapse();
                e.consume();
            } else if (e.getCode() == KeyCode.DOWN) {
                focusNextCard();
                e.consume();
            } else if (e.getCode() == KeyCode.UP) {
                focusPreviousCard();
                e.consume();
            } else if (e.getCode() == KeyCode.ENTER) {
                // Future: navigate map to selected instruction
                e.consume();
            }
        });

        // Title bar
        HBox titleBar = new HBox(uiScale.px(4));
        titleBar.setAlignment(Pos.CENTER_LEFT);
        titleBar.setPadding(uiScale.insets(8));
        titleBar.setStyle("-fx-background-color: #f0f0f0; -fx-background-radius: 6px 6px 0 0; -fx-border-color: #ddd; -fx-border-width: 0 0 1px 0;");

        Label title = new Label("Route");
        title.getStyleClass().add("route-panel-title");
        title.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        title.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(title, Priority.ALWAYS);

        Button closeButton = new Button("\u2715");
        closeButton.setStyle("-fx-background-color: transparent; -fx-text-fill: #999; -fx-font-size: " + uiScale.baseFontSize() + "px; -fx-cursor: hand;");
        closeButton.setMinSize(controlHeight, controlHeight);
        closeButton.setPrefSize(controlHeight, controlHeight);
        closeButton.setMaxSize(controlHeight, controlHeight);
        closeButton.setOnAction(e -> collapse());

        titleBar.getChildren().addAll(title, closeButton);

        // Start label (read-only) with search and favorite picker buttons
        startLabel = new Label("(tap to set start)");
        startLabel.getStyleClass().add("route-location-label");
        startLabel.setMaxWidth(Double.MAX_VALUE);
        startLabel.setMinHeight(controlHeight);
        startLabel.setPrefHeight(controlHeight);
        startLabel.setMaxHeight(controlHeight);
        startLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px;");
        startLabel.setOnMouseClicked(e -> pickStart());

        Button startSearchBtn = new Button();
        SVGPath searchIcon = new SVGPath();
        searchIcon.setContent("M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z");
        searchIcon.setScaleX(0.8);
        searchIcon.setScaleY(0.8);
        startSearchBtn.setGraphic(searchIcon);
        startSearchBtn.setMinSize(controlHeight, controlHeight);
        startSearchBtn.setPrefSize(controlHeight, controlHeight);
        startSearchBtn.setMaxSize(controlHeight, controlHeight);
        startSearchBtn.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-background-color: #e8e8e8; -fx-background-radius: 4px; -fx-cursor: hand;");
        startSearchBtn.setOnAction(e -> pickStart());

        Button startFavBtn = new Button();
        SVGPath starIcon = new SVGPath();
        starIcon.setContent("M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z");
        starIcon.setScaleX(0.8);
        starIcon.setScaleY(0.8);
        startFavBtn.setGraphic(starIcon);
        startFavBtn.setMinSize(controlHeight, controlHeight);
        startFavBtn.setPrefSize(controlHeight, controlHeight);
        startFavBtn.setMaxSize(controlHeight, controlHeight);
        startFavBtn.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-background-color: #e8e8e8; -fx-background-radius: 4px; -fx-cursor: hand;");
        startFavBtn.setOnAction(e -> pickStartFromFav());

        HBox startRow = new HBox(uiScale.px(4), startLabel, startSearchBtn, startFavBtn);
        startRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setHgrow(startLabel, Priority.ALWAYS);

        // Destination label (read-only) with search and favorite picker buttons
        destLabel = new Label("(tap to set destination)");
        destLabel.getStyleClass().add("route-location-label");
        destLabel.setMaxWidth(Double.MAX_VALUE);
        destLabel.setMinHeight(controlHeight);
        destLabel.setPrefHeight(controlHeight);
        destLabel.setMaxHeight(controlHeight);
        destLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px;");
        destLabel.setOnMouseClicked(e -> pickDest());

        Button destSearchBtn = new Button();
        SVGPath searchIcon2 = new SVGPath();
        searchIcon2.setContent("M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z");
        searchIcon2.setScaleX(0.8);
        searchIcon2.setScaleY(0.8);
        destSearchBtn.setGraphic(searchIcon2);
        destSearchBtn.setMinSize(controlHeight, controlHeight);
        destSearchBtn.setPrefSize(controlHeight, controlHeight);
        destSearchBtn.setMaxSize(controlHeight, controlHeight);
        destSearchBtn.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-background-color: #e8e8e8; -fx-background-radius: 4px; -fx-cursor: hand;");
        destSearchBtn.setOnAction(e -> pickDest());

        Button destFavBtn = new Button();
        SVGPath starIcon2 = new SVGPath();
        starIcon2.setContent("M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z");
        starIcon2.setScaleX(0.8);
        starIcon2.setScaleY(0.8);
        destFavBtn.setGraphic(starIcon2);
        destFavBtn.setMinSize(controlHeight, controlHeight);
        destFavBtn.setPrefSize(controlHeight, controlHeight);
        destFavBtn.setMaxSize(controlHeight, controlHeight);
        destFavBtn.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px; -fx-background-color: #e8e8e8; -fx-background-radius: 4px; -fx-cursor: hand;");
        destFavBtn.setOnAction(e -> pickDestFromFav());

        HBox destRow = new HBox(uiScale.px(4), destLabel, destSearchBtn, destFavBtn);
        destRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setHgrow(destLabel, Priority.ALWAYS);

        // Swap button
        Button swapButton = new Button("\u21C5");
        swapButton.getStyleClass().add("route-swap-button");
        swapButton.setMinSize(controlHeight, controlHeight);
        swapButton.setPrefSize(controlHeight, controlHeight);
        swapButton.setMaxSize(controlHeight, controlHeight);
        swapButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        swapButton.setOnAction(e -> swapStartDest());

        // Fields: rows stacked vertically, swap on the right
        VBox fieldsColumn = new VBox(uiScale.px(4), startRow, destRow);
        fieldsColumn.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(fieldsColumn, Priority.ALWAYS);
        HBox fieldsRow = new HBox(uiScale.px(4), fieldsColumn, swapButton);
        fieldsRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setMargin(fieldsRow, new Insets(uiScale.px(8), uiScale.px(10), 0, uiScale.px(10)));

        // Vehicle selector
        double vehicleBtnSize = controlHeight;
        String vehicleBtnBase = "-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-background-radius: 4px; -fx-cursor: hand; -fx-padding: 2px 6px;";

        carButton = new Button("Car");
        carButton.setMinSize(vehicleBtnSize, vehicleBtnSize);
        carButton.setPrefSize(vehicleBtnSize, vehicleBtnSize);
        carButton.setMaxSize(vehicleBtnSize, vehicleBtnSize);
        carButton.setStyle(vehicleBtnBase + " -fx-background-color: #4a90d9; -fx-text-fill: white;");
        carButton.setOnAction(e -> setVehicle(Vehicle.CAR));

        bicycleButton = new Button("Bike");
        bicycleButton.setMinSize(vehicleBtnSize, vehicleBtnSize);
        bicycleButton.setPrefSize(vehicleBtnSize, vehicleBtnSize);
        bicycleButton.setMaxSize(vehicleBtnSize, vehicleBtnSize);
        bicycleButton.setStyle(vehicleBtnBase + " -fx-background-color: #e8e8e8;");
        bicycleButton.setOnAction(e -> setVehicle(Vehicle.BICYCLE));

        pedestrianButton = new Button("Walk");
        pedestrianButton.setMinSize(vehicleBtnSize, vehicleBtnSize);
        pedestrianButton.setPrefSize(vehicleBtnSize, vehicleBtnSize);
        pedestrianButton.setMaxSize(vehicleBtnSize, vehicleBtnSize);
        pedestrianButton.setStyle(vehicleBtnBase + " -fx-background-color: #e8e8e8;");
        pedestrianButton.setOnAction(e -> setVehicle(Vehicle.PEDESTRIAN));

        Label vehicleLabel = new Label("Vehicle:");
        vehicleLabel.setStyle("-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-text-fill: #666;");

        HBox vehicleRow = new HBox(uiScale.px(4), vehicleLabel, carButton, bicycleButton, pedestrianButton);
        vehicleRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setMargin(vehicleRow, new Insets(uiScale.px(2), uiScale.px(10), 0, uiScale.px(10)));

        // Avoid options
        avoidFerriesCheck.setStyle("-fx-font-size: " + uiScale.smallFontSize() + "px;");
        avoidFerriesCheck.setOnAction(e -> {
            routingProfile = new RoutingProfile(routingProfile.vehicle,
                avoidTollsCheck.isSelected(), avoidFerriesCheck.isSelected(), false);
        });

        avoidTollsCheck.setStyle("-fx-font-size: " + uiScale.smallFontSize() + "px;");
        avoidTollsCheck.setOnAction(e -> {
            routingProfile = new RoutingProfile(routingProfile.vehicle,
                avoidTollsCheck.isSelected(), avoidFerriesCheck.isSelected(), false);
        });

        HBox avoidRow = new HBox(uiScale.px(8), avoidTollsCheck, avoidFerriesCheck);
        avoidRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setMargin(avoidRow, new Insets(uiScale.px(2), uiScale.px(10), 0, uiScale.px(10)));

        // Button row
        HBox buttonRow = new HBox(uiScale.px(4));
        buttonRow.setAlignment(Pos.CENTER_LEFT);
        HBox.setMargin(buttonRow, new Insets(uiScale.px(4), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        calculateButton = new Button("Calculate");
        calculateButton.getStyleClass().add("route-calculate-button");
        calculateButton.setMinHeight(controlHeight);
        calculateButton.setPrefHeight(controlHeight);
        calculateButton.setMaxHeight(controlHeight);
        calculateButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        calculateButton.setDisable(true);
        calculateButton.setOnAction(e -> calculateRoute());

        clearButton = new Button("Clear");
        clearButton.getStyleClass().add("route-clear-button");
        clearButton.setMinHeight(controlHeight);
        clearButton.setPrefHeight(controlHeight);
        clearButton.setMaxHeight(controlHeight);
        clearButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        clearButton.setOnAction(e -> clearRoute());

        buttonRow.getChildren().addAll(calculateButton, clearButton);

        // Instruction list (web-like card list, scrollable)
        instructionBox = new VBox(uiScale.px(4));
        instructionBox.setPadding(new Insets(uiScale.px(4), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        instructionBox.setFillWidth(true);

        instructionScroll = new ScrollPane(instructionBox);
        instructionScroll.setFitToWidth(true);
        instructionScroll.setHbarPolicy(ScrollPane.ScrollBarPolicy.NEVER);
        instructionScroll.setVbarPolicy(ScrollPane.ScrollBarPolicy.AS_NEEDED);
        instructionScroll.setPrefHeight(uiScale.px(60));
        instructionScroll.setMaxHeight(Double.MAX_VALUE);
        instructionScroll.setMinHeight(uiScale.px(60));
        instructionScroll.setVisible(false);
        instructionScroll.setManaged(true);
        instructionScroll.setStyle("-fx-background-color: transparent; -fx-border-color: transparent;");
        VBox.setMargin(instructionScroll, new Insets(uiScale.px(2), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        VBox.setVgrow(instructionScroll, Priority.ALWAYS);
        // routePanel itself must be allowed to grow vertically
        routePanel.setMaxHeight(Double.MAX_VALUE);

        // Progress box (hidden by default, managed so it stays inside routePanel)
        progressBox = new VBox(uiScale.px(8));
        progressBox.setAlignment(Pos.CENTER);
        progressBox.setPadding(uiScale.insets(12));
        progressBox.getStyleClass().add("route-progress-box");
        progressBox.setVisible(false);
        progressBox.setManaged(false);
        progressBox.setStyle("-fx-background-color: rgba(255,255,255,0.98); -fx-background-radius: 6px;");
        progressBox.setEffect(new javafx.scene.effect.DropShadow(8, 0, 0, javafx.scene.paint.Color.rgb(0, 0, 0, 0.2)));

        progressIndicator = new ProgressIndicator();
        progressLabel = new Label("Calculating route...");
        progressLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px;");
        cancelButton = new Button("Cancel");
        cancelButton.getStyleClass().add("route-cancel-button");
        cancelButton.setMinHeight(controlHeight);
        cancelButton.setPrefHeight(controlHeight);
        cancelButton.setMaxHeight(controlHeight);
        cancelButton.setStyle("-fx-font-size: " + uiScale.baseFontSize() + "px;");
        cancelButton.setOnAction(e -> client.cancelRoute());

        progressBox.getChildren().addAll(progressIndicator, progressLabel, cancelButton);

        navigationStatusLabel = new Label("");
        navigationStatusLabel.setWrapText(true);
        navigationStatusLabel.setStyle("-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-text-fill: #333;");
        navigationStatusLabel.setPadding(uiScale.insets(8));
        navigationStatusLabel.setVisible(false);
        navigationStatusLabel.setManaged(false);

        // Reroute status label (hidden by default)
        rerouteStatusLabel = new Label("");
        rerouteStatusLabel.setWrapText(true);
        rerouteStatusLabel.setAlignment(Pos.CENTER);
        rerouteStatusLabel.setMaxWidth(Double.MAX_VALUE);
        rerouteStatusLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px; -fx-text-fill: #4a90d9; -fx-font-weight: bold;");
        rerouteStatusLabel.setPadding(uiScale.insets(8));
        rerouteStatusLabel.setVisible(false);
        rerouteStatusLabel.setManaged(false);

        // Assemble panel (progressBox is an overlay child, not part of VBox)
        routePanel.getChildren().addAll(titleBar, fieldsRow, vehicleRow, avoidRow, buttonRow, navigationStatusLabel, rerouteStatusLabel, instructionScroll);

        // Add to layout: routePanel (bottom-right), progressBox (centered overlay)
        getChildren().addAll(routePanel, progressBox);

        // Position: bottom-right, above search button
        setAlignment(routePanel, Pos.BOTTOM_RIGHT);

        setPadding(overlayLayout.routeOverlayInsets(thumb)); // leave space for search button below

        // Listen to scene height changes for route panel sizing
        routePanelHeightListener = (obs, oldScene, newScene) -> {
            if (newScene != null) {
                newScene.heightProperty().addListener((obsH, oldH, newH) -> {
                    if (expanded) {
                        updateRoutePanelHeight(newH.doubleValue());
                    }
                });
            }
        };
        sceneProperty().addListener(routePanelHeightListener);
    }

    /** Get current route entry (null if none). */
    public RouteEntry getCurrentRoute() { return currentRoute; }

    /** Get the active routing profile. */
    public RoutingProfile getRoutingProfile() { return routingProfile; }

    /** Get the active vehicle type. */
    public Vehicle getVehicle() { return routingProfile.vehicle; }

    /** Whether a route is active. */
    public boolean isRouteActive() { return routeActive; }

    /** Get start lat. */
    public double getStartLat() { return startLocation != null ? startLocation.lat : 0.0; }

    /** Get start lon. */
    public double getStartLon() { return startLocation != null ? startLocation.lon : 0.0; }

    /** Get dest lat. */
    public double getDestLat() { return destLocation != null ? destLocation.lat : 0.0; }

    /** Get dest lon. */
    public double getDestLon() { return destLocation != null ? destLocation.lon : 0.0; }

    /** Cancel any in-progress routing (for shutdown). */
    public void cancelRouting() {
        if (client != null) {
            client.cancelRoute();
        }
    }

    /** Set start from map long-press. */
    public void setStartFromMap(double lat, double lon) {
        startLocation = new LocationEntry();
        startLocation.lat = lat;
        startLocation.lon = lon;
        startLocation.label = String.format("%.6f, %.6f", lat, lon);
        startLocation.matchQuality = "coordinate";
        updateStartLabel();
        updateCalculateButton();
    }

    /** Set destination from map long-press. */
    public void setDestFromMap(double lat, double lon) {
        destLocation = new LocationEntry();
        destLocation.lat = lat;
        destLocation.lon = lon;
        destLocation.label = String.format("%.6f, %.6f", lat, lon);
        destLocation.matchQuality = "coordinate";
        updateDestLabel();
        updateCalculateButton();
    }

    private void pickStart() {
        if (picker != null) {
            String initial = startLabel.getText();
            if (initial.startsWith("(tap")) {
                initial = "";
            }
            showLocationPicker("Select start", initial, entry -> {
                startLocation = entry;
                updateStartLabel();
                updateCalculateButton();
            });
        }
    }

    private void pickDest() {
        if (picker != null) {
            String initial = destLabel.getText();
            if (initial.startsWith("(tap")) {
                initial = "";
            }
            showLocationPicker("Select destination", initial, entry -> {
                destLocation = entry;
                updateDestLabel();
                updateCalculateButton();
            });
        }
    }

    private void pickStartFromFav() {
        if (favPicker != null) {
            favPicker.pickFavorite("Select start favorite", entry -> {
                startLocation = entry;
                updateStartLabel();
                updateCalculateButton();
            });
        }
    }

    private void pickDestFromFav() {
        if (favPicker != null) {
            favPicker.pickFavorite("Select destination favorite", entry -> {
                destLocation = entry;
                updateDestLabel();
                updateCalculateButton();
            });
        }
    }

    private void showLocationPicker(String prompt, String initialQuery,
                                    java.util.function.Consumer<LocationEntry> callback) {
        picker.pickLocation(prompt, initialQuery, callback);
    }

    private void updateStartLabel() {
        if (startLocation != null) {
            startLabel.setText(buildLocationSummary(startLocation));
        } else {
            startLabel.setText("(tap to set start)");
        }
    }

    private void updateDestLabel() {
        if (destLocation != null) {
            destLabel.setText(buildLocationSummary(destLocation));
        } else {
            destLabel.setText("(tap to set destination)");
        }
    }

    private String buildLocationSummary(LocationEntry entry) {
        StringBuilder sb = new StringBuilder();
        if (entry.label != null && !entry.label.isEmpty()) {
            sb.append(entry.label);
        }
        if (entry.adminRegionHierarchy != null && !entry.adminRegionHierarchy.isEmpty()) {
            sb.append(" \u2192 ").append(entry.adminRegionHierarchy);
        }
        if (sb.length() == 0) {
            sb.append(String.format("%.6f, %.6f", entry.lat, entry.lon));
        }
        return sb.toString();
    }

    private void swapStartDest() {
        LocationEntry tmp = startLocation;
        startLocation = destLocation;
        destLocation = tmp;
        updateStartLabel();
        updateDestLabel();
        if (routeActive) clearRoute();
        updateCalculateButton();
    }

    private void updateCalculateButton() {
        calculateButton.setDisable(startLocation == null || destLocation == null);
    }

    private void setVehicle(Vehicle vehicle) {
        routingProfile = new RoutingProfile(vehicle,
            avoidTollsCheck.isSelected(), avoidFerriesCheck.isSelected(), false);

        // Update button styles: highlight selected
        String activeStyle = "-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-background-radius: 4px; -fx-cursor: hand; -fx-padding: 2px 6px; -fx-background-color: #4a90d9; -fx-text-fill: white;";
        String inactiveStyle = "-fx-font-size: " + uiScale.smallFontSize() + "px; -fx-background-radius: 4px; -fx-cursor: hand; -fx-padding: 2px 6px; -fx-background-color: #e8e8e8;";

        carButton.setStyle(vehicle == Vehicle.CAR ? activeStyle : inactiveStyle);
        bicycleButton.setStyle(vehicle == Vehicle.BICYCLE ? activeStyle : inactiveStyle);
        pedestrianButton.setStyle(vehicle == Vehicle.PEDESTRIAN ? activeStyle : inactiveStyle);
    }

    private void calculateRoute() {
        if (startLocation == null || destLocation == null) {
            Log.error("RoutePanel: calculateRoute called with null start or dest");
            return;
        }
        Log.error("RoutePanel: calculating route from " + startLocation.label +
            " to " + destLocation.label);
        Log.error("RoutePanel:   start=(" + startLocation.lat + ", " + startLocation.lon +
            ") objOffset=" + startLocation.objectFileOffset +
            " refType=" + startLocation.refType +
            " type=" + startLocation.objectType);
        Log.error("RoutePanel:   dest=(" + destLocation.lat + ", " + destLocation.lon +
            ") objOffset=" + destLocation.objectFileOffset +
            " refType=" + destLocation.refType +
            " type=" + destLocation.objectType);
        showProgress();

        client.calculateRouteAsync(
            startLocation.lat, startLocation.lon,
            destLocation.lat, destLocation.lon,
            routingProfile,
            createRouteCallback());
    }

    private RouteCallback createRouteCallback() {
        return new RouteCallback() {
                @Override
                public void onProgress(int percent) {
                    Platform.runLater(() ->
                        progressLabel.setText("Calculating... " + percent + "%"));
                }

                @Override
                public void onSuccess(RouteEntry route) {
                    Platform.runLater(() -> {
                        hideProgress();
                        currentRoute = route;
                        routeActive = true;
                        showDescription(route);
                        if (onRouteReady != null) onRouteReady.run();
                    });
                }

                @Override
                public void onError(String message) {
                    Log.error("Route calculation error: " + message);
                    Platform.runLater(() -> {
                        hideProgress();
                        progressLabel.setText("Error: " + message);
                        progressLabel.getStyleClass().add("route-error-label");
                    });
                }

                @Override
                public void onCancel() {
                    Log.error("Route calculation cancelled");
                    Platform.runLater(() -> {
                        hideProgress();
                        progressLabel.setText("Cancelled");
                    });
                }
            };
    }

    private void clearRoute() {
        currentRoute = null;
        routeActive = false;
        instructionScroll.setVisible(false);
        instructionBox.getChildren().clear();
        if (onRouteReady != null) onRouteReady.run();
    }

    private void showDescription(RouteEntry route) {
        instructionBox.getChildren().clear();
        if (route.descriptions != null && route.descriptions.length > 0) {
            for (String d : route.descriptions) {
                if (d == null || d.startsWith("---")) continue;
                instructionBox.getChildren().add(createInstructionCard(d, ""));
            }
        } else {
            // Fallback: show distance/duration
            double distKm = route.distance / 1000.0;
            double durMin = route.duration / 60.0;
            String distStr = distKm < 1.0
                ? String.format("%.0f m", route.distance)
                : String.format("%.2f km", distKm);
            int hours = (int)(durMin / 60);
            int mins = (int)(durMin % 60);
            String durStr = hours > 0
                ? String.format("%d h %d min", hours, mins)
                : String.format("%d min", mins);
            instructionBox.getChildren().add(createInstructionCard("Distance: " + distStr + ", Duration: " + durStr, ""));
        }
        instructionScroll.setVisible(true);
    }

    public void toggleExpand() {
        if (expanded) {
            collapse();
        } else {
            expand();
        }
    }

    private void expand() {
        expanded = true;
        routePanel.setVisible(true);
        routePanel.setManaged(true);
        // Restore bottom padding for expanded route panel
        restoreOverlayPadding();
        // Rebind prefHeight to scene height
        sceneProperty().addListener(routePanelHeightListener);
        var scene = getScene();
        if (scene != null) {
            updateRoutePanelHeight(scene.getHeight());
        }
        setMouseTransparent(false);
        if (routeButton != null) {
            routeButton.setVisible(false);
        }
        routePanel.requestFocus();

        // Close on click outside — listen on scene
        Platform.runLater(() -> {
            var sc = getScene();
            if (sc != null) {
                sc.addEventFilter(MouseEvent.MOUSE_PRESSED, outsideHandler);
            }
        });

        FadeTransition fade = new FadeTransition(Duration.millis(200), routePanel);
        fade.setFromValue(0.0);
        fade.setToValue(1.0);

        TranslateTransition slide = new TranslateTransition(Duration.millis(200), routePanel);
        slide.setFromY(20.0);
        slide.setToY(0.0);

        ParallelTransition pt = new ParallelTransition(fade, slide);
        pt.play();
    }

    private void collapse() {
        expanded = false;
        setMouseTransparent(true);
        if (routeButton != null) {
            routeButton.setVisible(true);
        }
        // Remove scene listener
        var scene = getScene();
        if (scene != null) {
            scene.removeEventFilter(MouseEvent.MOUSE_PRESSED, outsideHandler);
        }
        FadeTransition fade = new FadeTransition(Duration.millis(150), routePanel);
        fade.setFromValue(1.0);
        fade.setToValue(0.0);
        fade.setOnFinished(e -> {
            routePanel.setVisible(false);
            routePanel.setManaged(false);
            // Unbind prefHeight so it doesn't affect StackPane layout
            routePanel.prefHeightProperty().unbind();
            routePanel.setPrefHeight(javafx.scene.layout.Region.USE_COMPUTED_SIZE);
        });
        fade.play();
    }

    private void restoreOverlayPadding() {
        double thumb = uiScale.thumbSize();
        setPadding(overlayLayout.routeOverlayInsets(thumb));
    }

    private void updateRoutePanelHeight(double sceneHeight) {
        double bottomReserve = uiScale.edgeMargin() + uiScale.thumbSize() + uiScale.buttonGap();
        double topReserve = uiScale.edgeMargin();
        double h = Math.max(uiScale.px(200), sceneHeight - topReserve - bottomReserve);
        routePanel.setPrefHeight(h);
    }

    private void focusNextCard() {
        var children = instructionBox.getChildren();
        if (children.isEmpty()) return;
        int current = -1;
        for (int i = 0; i < children.size(); i++) {
            if (children.get(i).isFocused()) {
                current = i;
                break;
            }
        }
        int next = current + 1;
        if (next >= children.size()) next = children.size() - 1;
        if (next >= 0) {
            children.get(next).requestFocus();
        }
    }

    private void focusPreviousCard() {
        var children = instructionBox.getChildren();
        if (children.isEmpty()) return;
        int current = children.size();
        for (int i = 0; i < children.size(); i++) {
            if (children.get(i).isFocused()) {
                current = i;
                break;
            }
        }
        int prev = current - 1;
        if (prev < 0) prev = 0;
        children.get(prev).requestFocus();
    }

    private void showProgress() {
        progressBox.setVisible(true);
        progressBox.setManaged(true);
        progressLabel.setText("Calculating route...");
        progressLabel.getStyleClass().remove("route-error-label");
        FadeTransition fade = new FadeTransition(Duration.millis(200), progressBox);
        fade.setFromValue(0.0);
        fade.setToValue(1.0);
        fade.play();
    }

    private void hideProgress() {
        FadeTransition fade = new FadeTransition(Duration.millis(150), progressBox);
        fade.setFromValue(1.0);
        fade.setToValue(0.0);
        fade.setOnFinished(e -> {
            progressBox.setVisible(false);
            progressBox.setManaged(false);
        });
        fade.play();
    }

    public void setNavigationActive(boolean active) {
        this.navigationActive = active;
        navigationStatusLabel.setVisible(active);
        navigationStatusLabel.setManaged(active);
        if (!active) {
            navigationStatusLabel.setText("");
        }
    }

    public void updateNavigationStatus(com.framstag.libosmscout.client.NavigationPosition position) {
        if (!navigationActive || position == null) return;
        StringBuilder sb = new StringBuilder();
        // Show vehicle type
        switch (routingProfile.vehicle) {
            case BICYCLE: sb.append("[Bike] "); break;
            case PEDESTRIAN: sb.append("[Walk] "); break;
            default: sb.append("[Car] "); break;
        }
        sb.append(routingProfile.vehicle.toString()).append("\n");
        sb.append("State: ").append(position.state.toString()).append("\n");
        sb.append(String.format("Pos: %.6f, %.6f", position.lat, position.lon));
        if (!Double.isNaN(position.bearing)) {
            sb.append(String.format(" | %.0f°", position.bearing));
        }
        navigationStatusLabel.setText(sb.toString());
    }

    public void updateArrivalEstimate(long arrivalEstimateMs, double remainingDistanceM) {
        if (!navigationActive) return;
        String text = navigationStatusLabel.getText();
        String arrival = java.time.format.DateTimeFormatter.ISO_TIME.format(
            java.time.LocalDateTime.ofInstant(
                java.time.Instant.ofEpochMilli(arrivalEstimateMs), java.time.ZoneId.systemDefault()));
        navigationStatusLabel.setText(text + "\nETA: " + arrival + " | remaining: " + formatDistance(remainingDistanceM));
    }

    public void updateCurrentSpeed(double speedKmH) {
        if (!navigationActive) return;
        String text = navigationStatusLabel.getText();
        if (speedKmH >= 0) {
            navigationStatusLabel.setText(text + "\nSpeed: " + String.format("%.0f", speedKmH) + " km/h");
        }
    }

    public void updateMaxAllowedSpeed(double maxSpeedKmH) {
        if (!navigationActive) return;
        String text = navigationStatusLabel.getText();
        if (maxSpeedKmH >= 0) {
            navigationStatusLabel.setText(text + "\nLimit: " + String.format("%.0f", maxSpeedKmH) + " km/h");
        }
    }

    public void updateLaneInfo(boolean oneway, int count, boolean suggested,
                               int suggestedFrom, int suggestedTo, String turn) {
        if (!navigationActive) return;
        String text = navigationStatusLabel.getText();
        StringBuilder sb = new StringBuilder(text).append("\nLanes: ").append(count);
        if (suggested) {
            sb.append(" (use ").append(suggestedFrom + 1).append("-").append(suggestedTo + 1).append(")");
        }
        if (!turn.isEmpty()) {
            sb.append(" turn: ").append(turn);
        }
        navigationStatusLabel.setText(sb.toString());
    }

    /**
     * Update the next-turn display with the current instruction.
     * Called from MainController on each position update.
     */
    public void updateInstructionList(RouteInstruction[] instructions) {
        if (instructions == null || instructions.length == 0) return;

        instructionBox.getChildren().clear();
        for (RouteInstruction instr : instructions) {
            instructionBox.getChildren().add(createInstructionCard(
                instr.shortDescription + " \u2192 " + formatDistance(instr.distanceTo),
                instr.streetName));
        }
        instructionScroll.setVisible(true);
    }

    /**
     * Show or hide the reroute status indicator.
     *
     * @param active true to show "Rerouting...", false to hide
     * @param failed true to show "Reroute failed" toast (auto-dismissed after 3s)
     */
    public void setRerouteStatus(boolean active, boolean failed) {
        if (failed) {
            rerouteStatusLabel.setText("Reroute failed");
            rerouteStatusLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px; -fx-text-fill: #c0392b; -fx-font-weight: bold;");
            rerouteStatusLabel.setVisible(true);
            rerouteStatusLabel.setManaged(true);

            // Auto-dismiss after 3 seconds
            if (rerouteToastTimer != null) {
                rerouteToastTimer.stop();
            }
            rerouteToastTimer = new javafx.animation.Timeline(
                new javafx.animation.KeyFrame(
                    javafx.util.Duration.seconds(3),
                    e -> {
                        rerouteStatusLabel.setVisible(false);
                        rerouteStatusLabel.setManaged(false);
                    }));
            rerouteToastTimer.setCycleCount(1);
            rerouteToastTimer.play();
            return;
        }

        if (active) {
            rerouteStatusLabel.setText("Rerouting\u2026");
            rerouteStatusLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px; -fx-text-fill: #4a90d9; -fx-font-weight: bold;");
            rerouteStatusLabel.setVisible(true);
            rerouteStatusLabel.setManaged(true);
        } else {
            rerouteStatusLabel.setVisible(false);
            rerouteStatusLabel.setManaged(false);
            if (rerouteToastTimer != null) {
                rerouteToastTimer.stop();
                rerouteToastTimer = null;
            }
        }
    }

    /**
     * Update the route panel with a reroute result.
     * Called from MainController after a successful reroute.
     */
    public void updateRerouteResult(RouteEntry route) {
        currentRoute = route;
        routeActive = true;
        showDescription(route);
    }

    /** Overload of createInstructionCard that takes two lines. */
    private HBox createInstructionCard(String primary, String secondary) {
        double baseFont = uiScale.baseFontSize();
        double bodyFont = uiScale.bodyFontSize();
        double smallFont = uiScale.smallFontSize();

        Label primaryLbl = new Label(primary);
        primaryLbl.setWrapText(true);
        primaryLbl.setStyle("-fx-font-size: " + bodyFont + "px; -fx-font-weight: bold; -fx-text-fill: #333;");

        Label secondaryLbl = new Label(secondary);
        secondaryLbl.setWrapText(true);
        secondaryLbl.setStyle("-fx-font-size: " + smallFont + "px; -fx-text-fill: #666;");

        VBox textBox = new VBox(uiScale.px(2), primaryLbl, secondaryLbl);
        textBox.setAlignment(Pos.CENTER_LEFT);
        HBox.setHgrow(textBox, Priority.ALWAYS);

        HBox card = new HBox(uiScale.px(10), textBox);
        card.setAlignment(Pos.CENTER_LEFT);
        card.setPadding(new Insets(uiScale.px(8), uiScale.px(10), uiScale.px(8), uiScale.px(10)));
        card.setStyle("-fx-background-color: #fafafa; -fx-background-radius: 6px; -fx-border-color: #eee; -fx-border-radius: 6px;");
        card.setMaxWidth(Double.MAX_VALUE);

        return card;
    }

    private static String formatDistance(double meters) {
        if (meters < 1000) {
            return String.format("%.0f m", meters);
        }
        return String.format("%.2f km", meters / 1000.0);
    }
}
