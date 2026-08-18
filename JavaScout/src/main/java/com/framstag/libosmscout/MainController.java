package com.framstag.libosmscout;

import com.framstag.libosmscout.client.BasemapManager;
import com.framstag.libosmscout.client.NavigationController;
import com.framstag.libosmscout.client.NavigationListener;
import com.framstag.libosmscout.client.NavigationPosition;
import com.framstag.libosmscout.client.NavigationState;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;
import com.framstag.libosmscout.client.TrackPoint;
import com.framstag.libosmscout.client.LaneTurn;
import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.ObjectDescription;
import com.framstag.libosmscout.client.PoiEntry;
import com.framstag.libosmscout.client.RouteInstruction;
import com.framstag.libosmscout.client.RouteCallback;
import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.TurnType;
import com.framstag.libosmscout.client.Vehicle;

import javafx.application.Platform;

import javafx.concurrent.Task;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.canvas.Canvas;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.ButtonType;
import javafx.scene.control.ComboBox;
import javafx.scene.control.ContextMenu;
import javafx.scene.control.Dialog;
import javafx.scene.control.DialogPane;
import javafx.scene.control.Label;
import javafx.scene.control.MenuItem;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.Node;
import javafx.scene.shape.SVGPath;
import javafx.stage.FileChooser;
import javafx.stage.Stage;
import javafx.scene.control.TextInputDialog;
import javafx.geometry.Insets;
import javafx.geometry.Pos;

import java.io.File;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ResourceBundle;
import java.util.function.Consumer;
import java.util.function.DoubleSupplier;

/**
 * Main controller for the JavaScout FXML layout.
 * <p>
 * Initialises the map renderer, interaction handlers, search overlay,
 * route panel, zoom controls, favorites management, and description overlay.
 */
public class MainController implements Initializable {

    @FXML
    private StackPane mapPanel;

    @FXML
    private Canvas mapCanvas;

    @FXML
    private Label placeholderLabel;

    @FXML
    private HBox statusBar;

    @FXML
    private Label dbPathLabel;

    @FXML
    private Label coordLabel;

    private UIScale uiScale;
    private String databaseDirectory;
    private String stylesheetDirectory;
    private String iconDirectory;

    @FXML
    private Label basemapStatusBarLabel;

    private OSMScoutClient client;
    private MapRenderer renderer;
    private MapInteractionHandler interactionHandler;
    private SearchOverlay searchOverlay;
    private PoiSearchOverlay poiSearchOverlay;
    private RoutePanel routePanel;
    private Config config;

    // Zoom controls
    private Button zoomInButton;
    private Button zoomOutButton;
    private StackPane zoomWrapper;

    // Overlay buttons
    private Button menuButton;
    private ContextMenu mainMenu;
    private Button searchButton;
    private Button routeButton;

    // Description overlay (created on demand)
    private DescriptionOverlay descriptionOverlay;

    // Long-press state
    private double longPressLat;
    private double longPressLon;

    // Navigation state
    private NavigationController navigationController;
    private TrackPlayer trackPlayer;
    private HBox trackToolbar;
    private TrackPoint[] importedTrackPoints;
    private boolean followMode = false;
    private Button followButton;

    // Map rotation mode
    private enum MapRotationMode {
        NORTH_UP,
        DRIVING_DIRECTION_UP
    }
    private MapRotationMode rotationMode = MapRotationMode.NORTH_UP;
    private Button compassButton;

    // Reroute state
    private static final long REROUTE_COOLDOWN_MS = 15_000;
    private long lastRerouteTime = 0;
    private boolean rerouting = false;
    private double routeDestLat;
    private double routeDestLon;
    private int rerouteGeneration = 0;

    // Auto-zoom by speed
    private record SpeedZoomLevel(double speedKmH, double magnification) {}
    private static final SpeedZoomLevel[] SPEED_ZOOM_TABLE = {
        new SpeedZoomLevel(0,   17.0), // stationary / walking (was 19, more zoomed out)
        new SpeedZoomLevel(6,   16.0), // slow jog (was 18)
        new SpeedZoomLevel(15,  15.0), // cycling / slow city (was 17)
        new SpeedZoomLevel(30,  14.0), // city driving (was 16)
        new SpeedZoomLevel(60,  14.0), // suburban / secondary (was 15)
        new SpeedZoomLevel(100, 14.0), // highway (was 13)
        new SpeedZoomLevel(140, 13.0), // fast (was 13)
        new SpeedZoomLevel(180, 13.0), // very fast (was 12, more zoomed in)
    };
    private boolean autoZoomEnabled = true;
    private boolean autoZoomSuspended = false;
    private int lastAutoZoomBand = -1;
    private double lastSpeedKmH = -1.0;
    private double lastGoodSpeedKmH = 20.0; // last plausible speed for spike filtering
    private double currentSmoothMag = 15.0; // routing-sensible default, not 5
    private double nextTurnDistanceM = Double.POSITIVE_INFINITY;
    private boolean turnZoomActive = false;
    private long lastZoomChangeTime = 0;

    // Road info lookup throttle (avoid DB query on every position update)
    private static final long ROAD_INFO_THROTTLE_MS = 2000;
    private long lastRoadInfoTime = 0;
    private double lastRoadInfoLat = Double.NaN;
    private double lastRoadInfoLon = Double.NaN;

    // Next-turn overlay + current road info (stacked in one container, top-left)
    private VBox nextTurnBox;
    private Label nextTurnIcon;
    private Label nextTurnDistance;
    private Label nextTurnStreet;
    private HBox nextNextRow;
    private Label nextNextDistance;
    private Label nextNextStreet;
    private Label currentRoadLabel;

    // Lane guidance in next-turn overlay
    private HBox nextTurnLaneRow;

    // Tracks whether client+renderer have been initialised
    private boolean initialised = false;
    private boolean initStarted = false;

    @Override
    public void initialize(URL location, ResourceBundle resources) {
        uiScale = new UIScale();
        config = new Config();

        // Bind canvas to panel size
        mapCanvas.widthProperty().bind(mapPanel.widthProperty());
        mapCanvas.heightProperty().bind(mapPanel.heightProperty());

        // Create overlay buttons
        createZoomControls();
        createMainMenuButton();
        createSearchButton();
        createRouteButton();

        // Keyboard shortcuts
        setupKeyboardShortcuts();

        // Canvas resize → re-render
        mapCanvas.widthProperty().addListener((obs, oldVal, newVal) -> reRender());
        mapCanvas.heightProperty().addListener((obs, oldVal, newVal) -> reRender());
    }

    private void reRender() {
        if (renderer != null) {
            renderer.requestRenderPreserveRoute(
                renderer.getLatitude(),
                renderer.getLongitude(),
                renderer.getMagnification());
        }
    }

    // ---- Initialisation (called from JavaScoutApp after setDatabaseDirectory) ----

    private void initClientAndRenderer() {
        if (initialised) return;

        // databaseDirectory is always set to the default download directory.
        // If no maps exist yet the scan will find none and the view stays empty.
        String stylesheets = stylesheetDirectory;
        if (stylesheets == null || stylesheets.isEmpty()) {
            // Default: stylesheets/ relative to working directory (project root)
            stylesheets = "stylesheets";
        }
        final String stylesheetsDir = stylesheets;

        final String iconDir;
        if (iconDirectory != null && !iconDirectory.isEmpty()) {
            String trimmed = iconDirectory.trim();
            iconDir = trimmed.endsWith("/") ? trimmed : trimmed + "/";
        } else {
            iconDir = "";
        }
        final String iconDirectoryForBuilder = iconDir;

        Task<Boolean> initTask = new Task<>() {
            @Override
            protected Boolean call() {
                try {
                    OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
                        .withMapLookupDirectories(databaseDirectory)
                        .withStyleSheetDirectory(stylesheetsDir)
                        .withPhysicalDpi(javafx.stage.Screen.getPrimary().getDpi())
                        .withUnits("metrics")
                        .withCustomPoiType("_route_start")
                        .withCustomPoiType("_route_end")
                        .withCustomPoiType("_route")
                        .withCustomPoiType("_favorite")
                        .withCustomPoiType("_search_selected")
                        .withCustomPoiType("_track");

                    if (!iconDirectoryForBuilder.isEmpty()) {
                        builder.withIconDirectory(iconDirectoryForBuilder);
                    }

                    // Check for installed basemap and pass to builder
                    Path basemapPath = java.nio.file.Paths.get(databaseDirectory, "basemap");
                    if (Files.isDirectory(basemapPath)) {
                        builder.withBasemapLookupDirectory(basemapPath.toAbsolutePath().toString());
                        Log.info("[MainController] basemap found at: " + basemapPath);
                    }

                    client = builder.build();
                    if (client == null) {
                        Log.error("MainController: client already initialised");
                        return false;
                    }

                    // Update basemap status bar
                    if (Files.isDirectory(basemapPath)) {
                        Platform.runLater(() -> basemapStatusBarLabel.setText("Basemap: active"));
                    }

                    return true;
                } catch (Exception e) {
                    Log.error("MainController: init error: " + e.getMessage());
                    e.printStackTrace();
                    return false;
                }
            }
        };

        initTask.setOnSucceeded(e -> {
            if (initTask.getValue()) {
                dbPathLabel.setText("DB: " + databaseDirectory);

                renderer = new MapRenderer(mapCanvas, client);

                // Resize canvas when panel size changes
                mapPanel.widthProperty().addListener((obs, oldVal, newVal) -> {
                    if (newVal != null && newVal.doubleValue() > 0) {
                        mapCanvas.widthProperty().unbind();
                        mapCanvas.setWidth(newVal.doubleValue());
                        if (renderer != null) {
                            renderer.requestRender(renderer.getLatitude(), renderer.getLongitude(), renderer.getMagnification());
                        }
                    }
                });
                mapPanel.heightProperty().addListener((obs, oldVal, newVal) -> {
                    if (newVal != null && newVal.doubleValue() > 0) {
                        mapCanvas.heightProperty().unbind();
                        mapCanvas.setHeight(newVal.doubleValue());
                        if (renderer != null) {
                            renderer.requestRender(renderer.getLatitude(), renderer.getLongitude(), renderer.getMagnification());
                        }
                    }
                });

                // Allow center to shrink below canvas size, clip overflow
                mapPanel.setMinSize(0, 0);
                javafx.scene.shape.Rectangle clip = new javafx.scene.shape.Rectangle();
                clip.widthProperty().bind(mapPanel.widthProperty());
                clip.heightProperty().bind(mapPanel.heightProperty());
                mapPanel.setClip(clip);
                boolean hasMaps = !client.getMapDownloadManager().getInstalledMaps().isEmpty();
                placeholderLabel.setVisible(!hasMaps);

                // View change listener updates coord label
                renderer.addViewChangeListener((lat, lon, mag, angle) ->
                    Platform.runLater(() -> {
                        coordLabel.setText(String.format("Lat: %.6f  Lon: %.6f  Z: %d", lat, lon, mag));
                        updateCompassRotation(angle);
                    }));

                // Create interaction handler
                int longPressTimeout = config.getLongPressTimeoutMs();
                interactionHandler = new MapInteractionHandler(
                    mapCanvas, renderer, longPressTimeout,
                    javafx.stage.Screen.getPrimary().getDpi());
                interactionHandler.setOnLongPress(this::onLongPress);
                interactionHandler.setOnInteractionStarted(this::onMapInteractionStarted);

                // Create follow-mode button
                createFollowButton();

                // Create compass button
                createCompassButton();

                // Create current road info overlay (above next-turn, top-left)
                createCurrentRoadOverlay();

                // Create next-turn overlay (content-sized, top-left)
                createNextTurnOverlay();

                // Create overlays that need client
                createSearchOverlay();
                createPoiSearchOverlay();
                createRoutePanel();

                // Initial render
                renderer.requestRender(
                    MapRenderer.DEFAULT_LATITUDE,
                    MapRenderer.DEFAULT_LONGITUDE,
                    MapRenderer.DEFAULT_MAGNIFICATION);

                // Load favorites
                loadFavorites();

                // Restore last map position
                restoreMapPosition();

                initialised = true;
            } else {
                dbPathLabel.setText("Failed to initialise: " + databaseDirectory);
            }
        });

        initTask.setOnFailed(e -> {
            Throwable err = initTask.getException();
            Log.error("MainController: init failed: " +
                (err != null ? err.getMessage() : "unknown"));
            dbPathLabel.setText("Init error");
        });

        Thread thread = new Thread(initTask, "client-init");
        thread.setDaemon(true);
        thread.start();
    }

    private void loadFavorites() {
        if (client == null) return;
        String favPath = config.getFavoritesFilePath();
        Task<Boolean> favTask = new Task<>() {
            @Override
            protected Boolean call() {
                return client.loadFavoriteLocations(favPath);
            }
        };
        favTask.setOnSucceeded(e -> {
            updateFavoriteMarkers();
            if (searchOverlay != null) {
                searchOverlay.refreshFavorites();
            }
        });
        favTask.setOnFailed(e -> Log.error("Failed to load favorites"));
        Thread thread = new Thread(favTask, "fav-load");
        thread.setDaemon(true);
        thread.start();
    }

    private void updateFavoriteMarkers() {
        if (client == null || renderer == null) return;
        var groups = client.getFavoriteGroups();
        if (groups == null) {
            renderer.setFavoriteLocations(null);
            return;
        }
        int count = 0;
        for (var group : groups) {
            if (group.favorites != null) {
                count += group.favorites.size();
            }
        }
        com.framstag.libosmscout.client.FavoriteLocation[] favArray =
            new com.framstag.libosmscout.client.FavoriteLocation[count];
        int idx = 0;
        for (var group : groups) {
            if (group.favorites != null) {
                for (var fav : group.favorites) {
                    favArray[idx++] = fav;
                }
            }
        }
        renderer.setFavoriteLocations(favArray);
    }

    private void createSearchOverlay() {
        searchOverlay = new SearchOverlay(client, uiScale, () -> {
            if (renderer != null) {
                renderer.requestRenderPreserveRoute(
                    searchOverlay.getMapCenterLat(),
                    searchOverlay.getMapCenterLon(),
                    searchOverlay.getTargetMagnification());
            }
        }, this::onSearchSelectionChanged, searchButton);
        searchOverlay.setMapCenter(
            MapRenderer.DEFAULT_LATITUDE,
            MapRenderer.DEFAULT_LONGITUDE);
        mapPanel.getChildren().add(searchOverlay);
    }

    private void onSearchSelectionChanged(LocationEntry entry) {
        if (renderer == null) return;
        if (entry != null) {
            renderer.setSearchSelected(entry.lat, entry.lon);
        } else {
            renderer.clearSearchSelected();
        }
    }

    private void createPoiSearchOverlay() {
        poiSearchOverlay = new PoiSearchOverlay(client, uiScale,
            () -> renderer != null ? renderer.getLatitude() : MapRenderer.DEFAULT_LATITUDE,
            () -> renderer != null ? renderer.getLongitude() : MapRenderer.DEFAULT_LONGITUDE,
            this::onPoiNavigate,
            this::showPoiDescription);
        mapPanel.getChildren().add(poiSearchOverlay);
    }

    private void onPoiNavigate(PoiEntry entry) {
        if (renderer == null) {
            return;
        }
        renderer.requestRenderPreserveRoute(entry.lat, entry.lon, renderer.getMagnification());
        renderer.setSearchSelected(entry.lat, entry.lon);
    }

    private void showPoiDescription(ObjectDescription desc) {
        if (desc == null || desc.getEntries().isEmpty()) {
            Alert alert = new Alert(Alert.AlertType.INFORMATION);
            alert.setTitle("Details");
            alert.setHeaderText(null);
            alert.setContentText("No description available");
            alert.show();
        } else {
            showDescriptionOverlay(desc);
        }
    }

    private void openPoiSearch() {
        if (poiSearchOverlay != null) {
            poiSearchOverlay.open();
        }
    }

    private void createRoutePanel() {
        routePanel = new RoutePanel(client, uiScale,
            this::onRouteChanged,
            this::pickLocationForRoute,
            this::pickFavoriteForRoute,
            routeButton);
        mapPanel.getChildren().add(routePanel);
    }

    private void createSearchButton() {
        double thumb = uiScale.thumbSize();

        SVGPath searchIcon = new SVGPath();
        searchIcon.setContent("M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z");
        searchIcon.setScaleX(1.2);
        searchIcon.setScaleY(1.2);

        searchButton = new Button();
        searchButton.setGraphic(searchIcon);
        searchButton.getStyleClass().add("search-overlay-button");
        searchButton.setMinSize(thumb, thumb);
        searchButton.setPrefSize(thumb, thumb);
        searchButton.setMaxSize(thumb, thumb);
        searchButton.setOnAction(e -> searchOverlay.toggleExpand());

        StackPane.setAlignment(searchButton, Pos.BOTTOM_RIGHT);
        StackPane.setMargin(searchButton,
            new Insets(0, uiScale.edgeMargin(), uiScale.edgeMargin(), 0));

        mapPanel.getChildren().add(searchButton);
    }

    private void createRouteButton() {
        double thumb = uiScale.thumbSize();

        SVGPath routeIcon = new SVGPath();
        routeIcon.setContent("M12 2C8.13 2 5 5.13 5 9c0 5.25 7 13 7 13s7-7.75 7-13c0-3.87-3.13-7-7-7zm0 9.5c-1.38 0-2.5-1.12-2.5-2.5s1.12-2.5 2.5-2.5 2.5 1.12 2.5 2.5-1.12 2.5-2.5 2.5z");
        routeIcon.setScaleX(1.2);
        routeIcon.setScaleY(1.2);

        routeButton = new Button();
        routeButton.setGraphic(routeIcon);
        routeButton.getStyleClass().add("route-overlay-button");
        routeButton.setMinSize(thumb, thumb);
        routeButton.setPrefSize(thumb, thumb);
        routeButton.setMaxSize(thumb, thumb);
        routeButton.setOnAction(e -> routePanel.toggleExpand());

        StackPane.setAlignment(routeButton, Pos.BOTTOM_RIGHT);
        StackPane.setMargin(routeButton,
            new Insets(0, uiScale.edgeMargin(),
                uiScale.edgeMargin() + 3 * thumb + 3 * uiScale.buttonGap(), 0));

        mapPanel.getChildren().add(routeButton);
    }

    private void createCompassButton() {
        double thumb = uiScale.thumbSize();

        // Compass icon: north-pointing triangle (red north, white south)
        SVGPath compassIcon = new SVGPath();
        compassIcon.setContent("M12 2L6 14l6-3 6 3L12 2zM12 22l6-10-6 3-6-3 6 10z");
        compassIcon.setScaleX(1.0);
        compassIcon.setScaleY(1.0);
        compassIcon.setStyle("-fx-fill: #d32f2f;");

        compassButton = new Button();
        compassButton.setGraphic(compassIcon);
        compassButton.getStyleClass().add("compass-overlay-button");
        compassButton.setMinSize(thumb, thumb);
        compassButton.setPrefSize(thumb, thumb);
        compassButton.setMaxSize(thumb, thumb);
        updateCompassButtonStyle();
        compassButton.setOnAction(e -> toggleRotationMode());

        StackPane.setAlignment(compassButton, Pos.BOTTOM_RIGHT);
        StackPane.setMargin(compassButton,
            new Insets(0, uiScale.edgeMargin(),
                uiScale.edgeMargin() + 2 * thumb + 2 * uiScale.buttonGap(), 0));

        mapPanel.getChildren().add(compassButton);
    }

    private void toggleRotationMode() {
        switch (rotationMode) {
            case NORTH_UP:
                rotationMode = MapRotationMode.DRIVING_DIRECTION_UP;
                break;
            case DRIVING_DIRECTION_UP:
                rotationMode = MapRotationMode.NORTH_UP;
                if (renderer != null) {
                    renderer.setAngle(0.0);
                }
                break;
        }
        updateCompassButtonStyle();
    }

    private void updateCompassButtonStyle() {
        if (compassButton == null) return;
        switch (rotationMode) {
            case NORTH_UP:
                compassButton.setStyle("-fx-background-color: #f0f0f0; -fx-text-fill: #333; -fx-background-radius: 50%;");
                break;
            case DRIVING_DIRECTION_UP:
                compassButton.setStyle("-fx-background-color: #4a90d9; -fx-text-fill: white; -fx-background-radius: 50%;");
                break;
        }
    }

    private void updateCompassRotation(double mapAngle) {
        if (compassButton == null) return;
        // Compass rotates opposite to map so north pointer stays accurate
        double compassDegrees = Math.toDegrees(mapAngle);
        compassButton.setRotate(compassDegrees);
    }

    private void createFollowButton() {
        double thumb = uiScale.thumbSize();

        SVGPath followIcon = new SVGPath();
        // Crosshair icon
        followIcon.setContent("M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm0-13c-.55 0-1 .45-1 1v4c0 .55.45 1 1 1s1-.45 1-1V8c0-.55-.45-1-1-1zm0 8c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3z");
        followIcon.setScaleX(1.0);
        followIcon.setScaleY(1.0);

        followButton = new Button();
        followButton.setGraphic(followIcon);
        followButton.getStyleClass().add("follow-overlay-button");
        followButton.setMinSize(thumb, thumb);
        followButton.setPrefSize(thumb, thumb);
        followButton.setMaxSize(thumb, thumb);
        updateFollowButtonStyle();
        followButton.setOnAction(e -> toggleFollowMode());

        StackPane.setAlignment(followButton, Pos.BOTTOM_RIGHT);
        StackPane.setMargin(followButton,
            new Insets(0, uiScale.edgeMargin(),
                uiScale.edgeMargin() + thumb + uiScale.buttonGap(), 0));

        mapPanel.getChildren().add(followButton);
    }

    private void createCurrentRoadOverlay() {
        double bodyFont = uiScale.bodyFontSize();

        currentRoadLabel = new Label("");
        currentRoadLabel.setWrapText(false);
        currentRoadLabel.setStyle(
            "-fx-font-size: " + bodyFont + "px;" +
            " -fx-text-fill: #333;" +
            " -fx-padding: 2px 6px;");
        currentRoadLabel.setVisible(false);
        currentRoadLabel.setManaged(false);
    }

    private void createNextTurnOverlay() {
        double smallFont = uiScale.smallFontSize();
        double bodyFont = uiScale.bodyFontSize();

        // Row 1: next instruction
        HBox row1 = new HBox(uiScale.px(4));
        row1.setAlignment(Pos.CENTER_LEFT);
        row1.setPadding(new Insets(uiScale.px(2), uiScale.px(6), uiScale.px(2), uiScale.px(6)));

        nextTurnIcon = new Label("\u27A1");
        nextTurnIcon.setMinSize(uiScale.px(18), uiScale.px(18));
        nextTurnIcon.setPrefSize(uiScale.px(18), uiScale.px(18));
        nextTurnIcon.setMaxSize(uiScale.px(18), uiScale.px(18));
        nextTurnIcon.setAlignment(Pos.CENTER);
        nextTurnIcon.setStyle("-fx-font-size: " + smallFont + "px; "
            + "-fx-background-color: #4a90d9; -fx-text-fill: white; -fx-background-radius: 9px;");

        nextTurnDistance = new Label("");
        nextTurnDistance.setStyle("-fx-font-size: " + bodyFont + "px; -fx-font-weight: bold;");
        nextTurnDistance.setMinWidth(uiScale.px(40));

        nextTurnStreet = new Label("");
        nextTurnStreet.setWrapText(false);
        nextTurnStreet.setStyle("-fx-font-size: " + bodyFont + "px; -fx-text-fill: #333;");

        row1.getChildren().addAll(nextTurnIcon, nextTurnDistance, nextTurnStreet);

        // Row 2: "next next" hint
        nextNextRow = new HBox(uiScale.px(4));
        nextNextRow.setAlignment(Pos.CENTER_LEFT);
        nextNextRow.setPadding(new Insets(0, uiScale.px(6), uiScale.px(2), uiScale.px(6 + 18 + 4)));
        nextNextRow.setVisible(false);
        nextNextRow.setManaged(false);

        nextNextDistance = new Label("");
        nextNextDistance.setStyle("-fx-font-size: " + smallFont + "px; -fx-text-fill: #666;");
        nextNextDistance.setMinWidth(uiScale.px(40));

        nextNextStreet = new Label("");
        nextNextStreet.setWrapText(false);
        nextNextStreet.setStyle("-fx-font-size: " + smallFont + "px; -fx-text-fill: #888;");

        nextNextRow.getChildren().addAll(nextNextDistance, nextNextStreet);

        // Row 3: lane guidance
        nextTurnLaneRow = new HBox(uiScale.px(2));
        nextTurnLaneRow.setAlignment(Pos.CENTER_LEFT);
        nextTurnLaneRow.setPadding(new Insets(0, uiScale.px(6), uiScale.px(2), uiScale.px(6 + 18 + 4)));
        nextTurnLaneRow.setVisible(false);
        nextTurnLaneRow.setManaged(false);

        // Single container VBox: current road on top, then next-turn rows below
        VBox box = new VBox(0);
        box.getChildren().addAll(currentRoadLabel, row1, nextTurnLaneRow, nextNextRow);
        box.setStyle("-fx-background-color: rgba(255,255,255,0.92); -fx-background-radius: 0 0 4px 0; -fx-border-color: #ccc; -fx-border-width: 0 1px 1px 0;");
        box.setVisible(false);
        box.setManaged(false);
        box.setMaxWidth(javafx.scene.layout.Region.USE_PREF_SIZE);
        box.setMaxHeight(javafx.scene.layout.Region.USE_PREF_SIZE);
        nextTurnBox = box;

        StackPane.setAlignment(nextTurnBox, Pos.TOP_LEFT);
        StackPane.setMargin(nextTurnBox, Insets.EMPTY);
        mapPanel.getChildren().add(nextTurnBox);
    }

    private void toggleFollowMode() {
        // Cycle: off -> follow+auto-zoom -> follow-only -> off
        if (!followMode) {
            followMode = true;
            autoZoomEnabled = true;
        } else if (autoZoomEnabled) {
            autoZoomEnabled = false;
        } else {
            followMode = false;
            autoZoomEnabled = true;
        }
        if (!followMode) {
            autoZoomSuspended = false;
        }
        updateFollowButtonStyle();
    }

    private void updateFollowButtonStyle() {
        if (followButton == null) return;
        String color;
        String label;
        if (!followMode) {
            color = "#f0f0f0; -fx-text-fill: #333;";
            label = "";
        } else if (autoZoomEnabled) {
            color = "#4a90d9; -fx-text-fill: white;";
            label = " AZ";
        } else {
            color = "#4a90d9; -fx-text-fill: white;";
            label = "";
        }
        followButton.setStyle("-fx-background-color: " + color +
            " -fx-background-radius: 6px;");
        followButton.setText(label);
    }

    private void onMapInteractionStarted() {
        if (followMode) {
            if (autoZoomEnabled) {
                // Suspend auto-zoom, keep follow mode
                autoZoomSuspended = true;
                lastAutoZoomBand = findBand(lastSpeedKmH);
            } else {
                followMode = false;
                updateFollowButtonStyle();
            }
        }
    }

    private void createTrackToolbar() {
        if (trackToolbar != null) {
            mapPanel.getChildren().remove(trackToolbar);
        }

        double controlHeight = uiScale.controlHeight();
        double baseFont = uiScale.baseFontSize();

        Button playButton = new Button("▶");
        playButton.setStyle("-fx-font-size: " + baseFont + "px;");
        playButton.setMinSize(controlHeight, controlHeight);
        playButton.setPrefSize(controlHeight, controlHeight);
        playButton.setOnAction(e -> {
            if (trackPlayer != null) trackPlayer.play();
        });

        Button pauseButton = new Button("⏸");
        pauseButton.setStyle("-fx-font-size: " + baseFont + "px;");
        pauseButton.setMinSize(controlHeight, controlHeight);
        pauseButton.setPrefSize(controlHeight, controlHeight);
        pauseButton.setOnAction(e -> {
            if (trackPlayer != null) trackPlayer.pause();
        });

        Button stopButton = new Button("⏹");
        stopButton.setStyle("-fx-font-size: " + baseFont + "px;");
        stopButton.setMinSize(controlHeight, controlHeight);
        stopButton.setPrefSize(controlHeight, controlHeight);
        stopButton.setOnAction(e -> {
            if (trackPlayer != null) trackPlayer.stop();
        });

        ComboBox<Double> speedBox = new ComboBox<>();
        speedBox.getItems().addAll(0.5, 1.0, 2.0, 5.0, 10.0);
        speedBox.setValue(1.0);
        speedBox.setStyle("-fx-font-size: " + baseFont + "px;");
        speedBox.valueProperty().addListener((obs, oldVal, newVal) -> {
            if (trackPlayer != null && newVal != null) {
                trackPlayer.setSpeedMultiplier(newVal);
            }
        });

        trackToolbar = new HBox(uiScale.px(4), playButton, pauseButton, stopButton,
            new Label("Speed:"), speedBox);
        trackToolbar.setAlignment(Pos.CENTER_LEFT);
        trackToolbar.setStyle("-fx-background-color: rgba(255,255,255,0.95); -fx-background-radius: 6px; -fx-padding: 4px;");
        trackToolbar.setMaxSize(javafx.scene.layout.Region.USE_PREF_SIZE, javafx.scene.layout.Region.USE_PREF_SIZE);

        StackPane.setAlignment(trackToolbar, Pos.BOTTOM_LEFT);
        StackPane.setMargin(trackToolbar,
            new Insets(uiScale.edgeMargin(), 0, 0, uiScale.edgeMargin()));
        mapPanel.getChildren().add(trackToolbar);
    }

    private void createOrRestartTrackPlayer() {
        if (importedTrackPoints == null || importedTrackPoints.length == 0 || navigationController == null) {
            return;
        }
        if (trackPlayer != null) {
            trackPlayer.stop();
        }
        trackPlayer = new TrackPlayer(importedTrackPoints, navigationController,
            status -> {});
        createTrackToolbar();
        trackPlayer.play();
    }

    private void removeTrackToolbar() {
        if (trackToolbar != null) {
            mapPanel.getChildren().remove(trackToolbar);
            trackToolbar = null;
        }
    }

    private void createZoomControls() {
        double thumb = uiScale.thumbSize();
        double baseFont = uiScale.baseFontSize();

        // Zoom in button
        zoomInButton = new Button("+");
        zoomInButton.getStyleClass().add("zoom-overlay-button");
        zoomInButton.setMinSize(thumb, thumb);
        zoomInButton.setPrefSize(thumb, thumb);
        zoomInButton.setMaxSize(thumb, thumb);
        zoomInButton.setStyle("-fx-font-size: " + baseFont + "px;");
        zoomInButton.setOnAction(e -> {
            if (renderer != null) {
                int mag = Math.min(18, renderer.getMagnification() + 1);
                renderer.requestRenderPreserveRoute(
                    renderer.getLatitude(),
                    renderer.getLongitude(),
                    mag);
            }
        });

        // Zoom out button
        zoomOutButton = new Button("\u2212");
        zoomOutButton.getStyleClass().add("zoom-overlay-button");
        zoomOutButton.setMinSize(thumb, thumb);
        zoomOutButton.setPrefSize(thumb, thumb);
        zoomOutButton.setMaxSize(thumb, thumb);
        zoomOutButton.setStyle("-fx-font-size: " + baseFont + "px;");
        zoomOutButton.setOnAction(e -> {
            if (renderer != null) {
                int mag = Math.max(0, renderer.getMagnification() - 1);
                renderer.requestRenderPreserveRoute(
                    renderer.getLatitude(),
                    renderer.getLongitude(),
                    mag);
            }
        });

        // Stack zoom buttons vertically, top-right corner
        VBox zoomBox = new VBox(uiScale.px(4));
        zoomBox.setAlignment(Pos.TOP_RIGHT);
        zoomBox.getChildren().addAll(zoomInButton, zoomOutButton);
        // Prevent zoomBox from expanding to fill the entire StackPane wrapper
        zoomBox.setMaxSize(javafx.scene.layout.Region.USE_PREF_SIZE, javafx.scene.layout.Region.USE_PREF_SIZE);

        // Wrap in a StackPane so alignment works independently of other children
        zoomWrapper = new StackPane(zoomBox);
        zoomWrapper.setPickOnBounds(false);
        StackPane.setAlignment(zoomWrapper, Pos.TOP_RIGHT);
        // Use same edge margin as search/route buttons for consistent spacing
        StackPane.setMargin(zoomWrapper, new Insets(uiScale.edgeMargin(), uiScale.edgeMargin(), 0, 0));
        // Align zoomBox to top-right within the wrapper so buttons stay at top-right
        StackPane.setAlignment(zoomBox, Pos.TOP_RIGHT);

        mapPanel.getChildren().add(zoomWrapper);
    }

    private void createMainMenuButton() {
        double thumb = uiScale.thumbSize();

        // Hamburger icon (three horizontal lines)
        SVGPath menuIcon = new SVGPath();
        menuIcon.setContent("M3 18h18v-2H3v2zm0-5h18v-2H3v2zm0-7v2h18V6H3z");
        menuIcon.setScaleX(1.0);
        menuIcon.setScaleY(1.0);

        menuButton = new Button();
        menuButton.setGraphic(menuIcon);
        menuButton.getStyleClass().add("menu-overlay-button");
        menuButton.setMinSize(thumb, thumb);
        menuButton.setPrefSize(thumb, thumb);
        menuButton.setMaxSize(thumb, thumb);

        mainMenu = new ContextMenu();
        mainMenu.setAutoHide(true);

        MenuItem favoritesItem = new MenuItem("Favorites");
        favoritesItem.setOnAction(e -> openFavoritesDialog());

        MenuItem importTrackItem = new MenuItem("Import GPX Track…");
        importTrackItem.setOnAction(e -> importGpxTrack());

        MenuItem poiSearchItem = new MenuItem("Search POIs…");
        poiSearchItem.setOnAction(e -> openPoiSearch());

        MenuItem liveGpsItem = new MenuItem("Simulate GPS Fix…");
        liveGpsItem.setOnAction(e -> openLiveGpsDialog());

        MenuItem downloadMapsItem = new MenuItem("Download Maps…");
        downloadMapsItem.setOnAction(e -> onDownloadMaps());

        mainMenu.getItems().addAll(favoritesItem, importTrackItem, poiSearchItem, liveGpsItem, downloadMapsItem);

        menuButton.setOnAction(e -> {
            if (mainMenu.isShowing()) {
                mainMenu.hide();
            } else {
                mainMenu.show(menuButton,
                    javafx.geometry.Side.BOTTOM,
                    0, 0);
            }
        });

        // Dismiss menu on Escape via the button's scene listener
        menuButton.sceneProperty().addListener((obs, oldScene, newScene) -> {
            if (newScene != null) {
                newScene.addEventFilter(KeyEvent.KEY_PRESSED, ev -> {
                    if (ev.getCode() == KeyCode.ESCAPE && mainMenu.isShowing()) {
                        mainMenu.hide();
                        ev.consume();
                    }
                });

                // Also dismiss the menu when the user presses the mouse anywhere
                // outside the menu button (the menu popup is a separate window,
                // so its own item clicks are not delivered to this scene).
                newScene.addEventFilter(MouseEvent.MOUSE_PRESSED, ev -> {
                    if (mainMenu.isShowing()
                        && ev.getTarget() instanceof Node targetNode
                        && !isDescendantOf(menuButton, targetNode)) {
                        mainMenu.hide();
                    }
                });
            }
        });

        StackPane.setAlignment(menuButton, Pos.BOTTOM_RIGHT);
        StackPane.setMargin(menuButton,
            new Insets(0, uiScale.edgeMargin(),
                uiScale.edgeMargin() + 4 * thumb + 4 * uiScale.buttonGap(), 0));

        mapPanel.getChildren().add(menuButton);
    }

    private void openLiveGpsDialog() {
        if (navigationController == null) {
            Log.error("[MainController] no active navigation session");
            return;
        }

        TextInputDialog dialog = new TextInputDialog(String.format("%.6f,%.6f",
            renderer != null ? renderer.getLatitude() : MapRenderer.DEFAULT_LATITUDE,
            renderer != null ? renderer.getLongitude() : MapRenderer.DEFAULT_LONGITUDE));
        dialog.setTitle("Simulate GPS Fix");
        dialog.setHeaderText("Enter latitude,longitude");
        dialog.setContentText("Position:");

        dialog.showAndWait().ifPresent(input -> {
            String[] parts = input.split("[,;\\s]+");
            if (parts.length != 2) {
                Log.error("[MainController] invalid GPS input: " + input);
                return;
            }
            try {
                double lat = Double.parseDouble(parts[0]);
                double lon = Double.parseDouble(parts[1]);
                navigationController.processLocation(lat, lon, -1.0, -1.0,
                    System.currentTimeMillis());
            } catch (NumberFormatException e) {
                Log.error("[MainController] invalid GPS coordinates: " + input);
            }
        });
    }

    private void importGpxTrack() {
        if (client == null) {
            Log.error("[MainController] cannot import track: client not initialised");
            return;
        }

        Stage stage = (Stage) mapPanel.getScene().getWindow();
        FileChooser fileChooser = new FileChooser();
        fileChooser.setTitle("Import GPX Track");
        fileChooser.getExtensionFilters().add(
            new FileChooser.ExtensionFilter("GPX files", "*.gpx"));

        File selected = fileChooser.showOpenDialog(stage);
        if (selected == null) {
            return;
        }

        Task<TrackPoint[]> importTask = new Task<>() {
            @Override
            protected TrackPoint[] call() {
                return client.importGpxTrack(selected.getAbsolutePath());
            }
        };

        importTask.setOnSucceeded(e -> {
            TrackPoint[] points = importTask.getValue();
            if (points == null || points.length == 0) {
                Log.error("[MainController] no track points imported from " + selected);
                return;
            }
            importedTrackPoints = points;
            if (renderer != null) {
                renderer.setTrackPoints(points);
            }
            createOrRestartTrackPlayer();
        });

        importTask.setOnFailed(e -> {
            Throwable err = importTask.getException();
            Log.error("[MainController] GPX import failed: " +
                (err != null ? err.getMessage() : "unknown"));
        });

        Thread thread = new Thread(importTask, "gpx-import");
        thread.setDaemon(true);
        thread.start();
    }

    private void setupKeyboardShortcuts() {
        mapPanel.addEventHandler(KeyEvent.KEY_PRESSED, e -> {
            // Ctrl+F or Cmd+F: open search
            if (e.getCode() == KeyCode.F &&
                (e.isControlDown() || e.isMetaDown())) {
                if (searchOverlay != null) {
                    searchOverlay.openSearch();
                }
                e.consume();
            }
            // Ctrl+R or Cmd+R: toggle route panel
            if (e.getCode() == KeyCode.R &&
                (e.isControlDown() || e.isMetaDown())) {
                if (routePanel != null) {
                    routePanel.toggleExpand();
                }
                e.consume();
            }
        });
    }

    // ---- Route callbacks ----

    private void pickLocationForRoute(String prompt, String initialQuery,
                                       Consumer<LocationEntry> callback) {
        if (searchOverlay != null) {
            searchOverlay.pickForRoute(prompt, initialQuery, callback);
        }
    }

    private void pickFavoriteForRoute(String prompt,
                                       Consumer<LocationEntry> callback) {
        if (client == null) return;
        Stage stage = (Stage) mapPanel.getScene().getWindow();
        FavoritePickerDialog dialog = new FavoritePickerDialog(
            stage, client, uiScale, callback);
        dialog.show();
    }

    private void onRouteChanged() {
        if (renderer == null) return;

        if (routePanel.isRouteActive() && routePanel.getCurrentRoute() != null) {
            var route = routePanel.getCurrentRoute();
            renderer.requestRender(
                renderer.getLatitude(),
                renderer.getLongitude(),
                renderer.getMagnification(),
                route.latitudes,
                route.longitudes);

            // Store destination coordinates for rerouting
            if (route.latitudes != null && route.latitudes.length > 0) {
                int last = route.latitudes.length - 1;
                routeDestLat = route.latitudes[last];
                routeDestLon = route.longitudes[last];
            }

            if (navigationController == null && route.routeHandle != 0) {
                startNavigationOnCurrentRoute();
            }
        } else {
            renderer.clearRouteOverlay();
            renderer.requestRenderPreserveRoute(
                renderer.getLatitude(),
                renderer.getLongitude(),
                renderer.getMagnification());
            stopNavigation();
        }
    }

    private void startNavigationOnCurrentRoute() {
        if (client == null || routePanel == null) return;
        var route = routePanel.getCurrentRoute();
        if (route == null || route.routeHandle == 0) {
            Log.error("[MainController] no route handle for navigation");
            return;
        }
        stopNavigation();
        navigationController = client.startNavigation(route.routeHandle,
            routePanel.getVehicle(), createNavigationListener());
        if (navigationController != null) {
            followMode = true;
            autoZoomEnabled = true;
            autoZoomSuspended = false;
            lastAutoZoomBand = -1;
            updateFollowButtonStyle();
            routePanel.setNavigationActive(true);
            if (nextTurnBox != null) {
                nextTurnBox.setVisible(true);
                nextTurnBox.setManaged(true);
            }
            createOrRestartTrackPlayer();
        } else {
            Log.error("[MainController] failed to start navigation");
        }
    }

    private void stopNavigation() {
        // Cancel any in-progress reroute
        if (rerouting) {
            rerouting = false;
            if (client != null) {
                client.cancelRoute();
            }
            if (routePanel != null) {
                routePanel.setRerouteStatus(false, false);
            }
        }

        if (navigationController != null) {
            navigationController.stop();
            navigationController = null;
        }
        if (trackPlayer != null) {
            trackPlayer.stop();
            trackPlayer = null;
        }
        removeTrackToolbar();
        if (renderer != null) {
            renderer.clearCurrentLocation();
            renderer.clearTrack();
        }
        if (routePanel != null) {
            routePanel.setNavigationActive(false);
        }
        if (nextTurnBox != null) {
            nextTurnBox.setVisible(false);
            nextTurnBox.setManaged(false);
        }
        if (currentRoadLabel != null) {
            currentRoadLabel.setVisible(false);
            currentRoadLabel.setManaged(false);
        }
        followMode = false;
        autoZoomEnabled = true;
        autoZoomSuspended = false;
        lastAutoZoomBand = -1;
        lastSpeedKmH = -1.0;
        updateFollowButtonStyle();
    }

    private void updateCurrentRoadOverlay(CurrentRoadInfo info) {
        if (currentRoadLabel == null || nextTurnBox == null) return;

        if (info == null || !info.hasInfo()) {
            currentRoadLabel.setVisible(false);
            currentRoadLabel.setManaged(false);
            return;
        }

        currentRoadLabel.setText(info.toDisplayString());
        currentRoadLabel.setVisible(true);
        currentRoadLabel.setManaged(true);
        nextTurnBox.setVisible(true);
        nextTurnBox.setManaged(true);
    }

    /**
     * Look up road info at the given coordinate using the description service.
     * Throttled to avoid DB queries on every position update.
     */
    private void updateRoadInfoFromPosition(double lat, double lon) {
        if (client == null) return;

        long now = System.currentTimeMillis();
        if (now - lastRoadInfoTime < ROAD_INFO_THROTTLE_MS) {
            return;
        }
        // Also skip if position hasn't moved significantly (~50m)
        if (!Double.isNaN(lastRoadInfoLat) && !Double.isNaN(lastRoadInfoLon)) {
            double dx = lat - lastRoadInfoLat;
            double dy = lon - lastRoadInfoLon;
            if (dx * dx + dy * dy < 0.0005 * 0.0005) { // ~50m at mid-latitudes
                return;
            }
        }

        lastRoadInfoTime = now;
        lastRoadInfoLat = lat;
        lastRoadInfoLon = lon;

        javafx.concurrent.Task<CurrentRoadInfo> task = new javafx.concurrent.Task<>() {
            @Override
            protected CurrentRoadInfo call() {
                var desc = client.getDescription(lat, lon);
                if (desc == null) return null;

                String ref = "";
                String typeName = "";
                String name = "";
                for (var entry : desc.getEntries()) {
                    if (!"General".equals(entry.sectionKey)) continue;
                    switch (entry.labelKey) {
                        case "NameRef" -> ref = entry.value;
                        case "Type" -> typeName = entry.value;
                        case "Name" -> name = entry.value;
                    }
                }
                return new CurrentRoadInfo(ref, typeName, name);
            }
        };

        task.setOnSucceeded(e -> {
            CurrentRoadInfo info = task.getValue();
            Platform.runLater(() -> updateCurrentRoadOverlay(info));
        });

        task.setOnFailed(e -> {
            Throwable err = task.getException();
            if (err != null) {
                Log.error("[MainController] road info lookup failed: " + err.getMessage());
            }
        });

        Thread thread = new Thread(task, "road-info-lookup");
        thread.setDaemon(true);
        thread.start();
    }

    private void updateNextTurnOverlay(RouteInstruction instruction) {
        if (instruction == null || nextTurnBox == null) return;

        // Row 1: next instruction
        if (instruction.distanceTo > 0) {
            nextTurnDistance.setText(formatDistance(instruction.distanceTo));
        } else {
            nextTurnDistance.setText("");
        }

        // Vehicle-adaptive icon: show vehicle icon + turn arrow
        String vehicleIcon = "";
        if (routePanel != null) {
            switch (routePanel.getVehicle()) {
                case BICYCLE: vehicleIcon = "Bike "; break;
                case PEDESTRIAN: vehicleIcon = "Walk "; break;
                default: vehicleIcon = ""; break;
            }
        }
        nextTurnIcon.setText(vehicleIcon + turnTypeToIcon(instruction.turnType));

        String text;
        if (instruction.description != null && !instruction.description.isEmpty()) {
            text = instruction.description;
        } else if (instruction.shortDescription != null && !instruction.shortDescription.isEmpty()) {
            text = instruction.shortDescription;
        } else if (instruction.streetName != null && !instruction.streetName.isEmpty()) {
            text = instruction.streetName;
        } else {
            text = "";
        }
        nextTurnStreet.setText(text);

        // Row 2: "next next" hint
        if (instruction.hasNextNext()) {
            nextNextDistance.setText(formatDistance(instruction.nextNextDistanceTo));
            String nnText;
            if (instruction.nextNextDescription != null && !instruction.nextNextDescription.isEmpty()) {
                nnText = instruction.nextNextDescription;
            } else if (instruction.nextNextShortDescription != null && !instruction.nextNextShortDescription.isEmpty()) {
                nnText = instruction.nextNextShortDescription;
            } else {
                nnText = "";
            }
            nextNextStreet.setText(nnText);
            nextNextRow.setVisible(true);
            nextNextRow.setManaged(true);
        } else {
            nextNextRow.setVisible(false);
            nextNextRow.setManaged(false);
        }
    }

    private void updateNextTurnLanes(boolean oneway, int count, boolean suggested,
                                      int suggestedFrom, int suggestedTo,
                                      LaneTurn[] turns) {
        if (nextTurnLaneRow == null) return;

        nextTurnLaneRow.getChildren().clear();

        if (count <= 0 || turns == null || turns.length == 0) {
            nextTurnLaneRow.setVisible(false);
            nextTurnLaneRow.setManaged(false);
            return;
        }

        for (int i = 0; i < turns.length && i < count; i++) {
            if (i > 0 && !oneway) {
                Label divider = new Label("|");
                divider.setStyle("-fx-text-fill: #ccc; -fx-font-size: " + uiScale.smallFontSize() + "px;");
                nextTurnLaneRow.getChildren().add(divider);
            }
            boolean isSuggested = suggested && i >= suggestedFrom && i <= suggestedTo;
            String arrow = laneTurnToArrow(turns[i]);
            String color = isSuggested ? "#4a90d9" : "#999";
            Label arrowLabel = new Label(arrow);
            arrowLabel.setStyle("-fx-font-size: " + uiScale.bodyFontSize() + "px; -fx-text-fill: " + color + "; -fx-font-weight: " + (isSuggested ? "bold" : "normal") + ";");
            nextTurnLaneRow.getChildren().add(arrowLabel);
        }
        nextTurnLaneRow.setVisible(true);
        nextTurnLaneRow.setManaged(true);
    }

    private static String laneTurnToArrow(LaneTurn turn) {
        switch (turn) {
            case LEFT:                return "\u2190";
            case SLIGHTLY_LEFT:       return "\u2196";
            case SHARP_LEFT:          return "\u21A9";
            case RIGHT:               return "\u2192";
            case SLIGHTLY_RIGHT:      return "\u2197";
            case SHARP_RIGHT:         return "\u21AA";
            case LEFT_AND_STRAIGHT:   return "\u219E\u2190";
            case STRAIGHT_AND_RIGHT:  return "\u2192\u219E";
            case MERGE_TO_LEFT:       return "\u2190\u2192";
            case MERGE_TO_RIGHT:      return "\u2192\u2190";
            case STRAIGHT_ON:
            default:                  return "\u2191";
        }
    }

    private static String formatDistance(double meters) {
        if (meters < 1000) {
            return String.format("%.0f m", meters);
        }
        return String.format("%.2f km", meters / 1000.0);
    }

    private static String turnTypeToIcon(TurnType turnType) {
        if (turnType == null) return "\u27A1";
        switch (turnType) {
            case SHARP_LEFT:     return "\u2B05\uFE0F";
            case LEFT:           return "\u2190";
            case SLIGHTLY_LEFT:  return "\u2199\uFE0F";
            case STRAIGHT_ON:    return "\u2B06\uFE0F";
            case SLIGHTLY_RIGHT: return "\u2198\uFE0F";
            case RIGHT:          return "\u2192";
            case SHARP_RIGHT:    return "\u27A1\uFE0F";
            case TARGET_REACHED: return "\u2B50";
            case ROUNDABOUT_ENTER: return "\u27F3";
            case ROUNDABOUT_LEAVE: return "\u27F2";
            case MOTORWAY_ENTER: return "\uD83D\uDEE3";
            default:             return "\u27A1";
        }
    }

    // ---- Auto-zoom by speed ----

    /**
     * Compute the target magnification for a given speed using linear interpolation
     * over the SPEED_ZOOM_TABLE. Clamps to table bounds.
     */
    private static double computeSpeedZoom(double speedKmH) {
        if (speedKmH < 0) {
            return SPEED_ZOOM_TABLE[0].magnification();
        }
        SpeedZoomLevel[] table = SPEED_ZOOM_TABLE;
        if (speedKmH <= table[0].speedKmH()) {
            return table[0].magnification();
        }
        if (speedKmH >= table[table.length - 1].speedKmH()) {
            return table[table.length - 1].magnification();
        }
        for (int i = 0; i < table.length - 1; i++) {
            SpeedZoomLevel a = table[i];
            SpeedZoomLevel b = table[i + 1];
            if (speedKmH >= a.speedKmH() && speedKmH <= b.speedKmH()) {
                double t = (speedKmH - a.speedKmH()) / (b.speedKmH() - a.speedKmH());
                return a.magnification() + t * (b.magnification() - a.magnification());
            }
        }
        return table[table.length - 1].magnification();
    }

    /**
     * Find the table band index for a given speed.
     * Returns the index of the first entry whose speedKmH >= the given speed,
     * or the last index if speed exceeds all entries.
     */
    private static int findBand(double speedKmH) {
        if (speedKmH < 0) {
            return 0;
        }
        SpeedZoomLevel[] table = SPEED_ZOOM_TABLE;
        for (int i = 0; i < table.length; i++) {
            if (speedKmH <= table[i].speedKmH()) {
                return i;
            }
        }
        return table.length - 1;
    }

    private NavigationListener createNavigationListener() {
        final int capturedGeneration = rerouteGeneration;
        return new NavigationListener() {
            private boolean isStale() {
                return capturedGeneration != rerouteGeneration;
            }

            @Override
            public void onPositionEstimate(NavigationPosition position) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (renderer != null) {
                        renderer.setCurrentLocation(position.lat, position.lon,
                            position.bearing, position.accuracy);
                    }
                    if (followMode && renderer != null) {
                        double angle = 0.0;
                        if (rotationMode == MapRotationMode.DRIVING_DIRECTION_UP
                            && !Double.isNaN(position.bearing)) {
                            // MercatorProjection angle is counter-clockwise (math convention),
                            // bearing is clockwise (navigation convention). Negate.
                            angle = -Math.toRadians(position.bearing);
                        }
                        double mag = currentSmoothMag;
                        if (autoZoomEnabled) {
                            // Use default speed 20 km/h if speed unknown, so initial zoom fits
                            // Reject speed spikes (>150 km/h) — use last good speed instead
                            // SpeedAgent can produce bogus values (e.g. 392 km/h after tunnel gap)
                            double speed;
                            if (lastSpeedKmH >= 0 && lastSpeedKmH <= 150.0) {
                                speed = lastSpeedKmH;
                                lastGoodSpeedKmH = lastSpeedKmH;
                            } else if (lastSpeedKmH >= 0) {
                                Log.error("[AutoZoom] speed spike rejected: "
                                    + String.format("%.1f", lastSpeedKmH)
                                    + " using " + String.format("%.1f", lastGoodSpeedKmH));
                                speed = lastGoodSpeedKmH;
                            } else {
                                speed = 20.0;
                            }
                            double targetMag = computeSpeedZoom(speed);

                            // On first position estimate, jump directly to target
                            // instead of smoothing from DEFAULT_MAGNIFICATION (5)
                            if (lastSpeedKmH < 0) {
                                Log.error("[AutoZoom] initial speed=" + String.format("%.1f", speed)
                                    + " targetMag=" + String.format("%.1f", targetMag));
                                mag = targetMag;
                                currentSmoothMag = targetMag;
                            }

                            // Turn-aware zoom: zoom in before close turns so driver sees detail
                            // Active from 600m before turn until 600m after (same threshold)
                            boolean wasTurnZoomActive = turnZoomActive;
                            if (nextTurnDistanceM < 600) {
                                turnZoomActive = true;
                            } else if (nextTurnDistanceM > 1200) {
                                turnZoomActive = false;
                            }
                            if (turnZoomActive != wasTurnZoomActive) {
                                Log.error("[AutoZoom] turnZoom " + (turnZoomActive ? "ACTIVE" : "INACTIVE")
                                    + " dist=" + String.format("%.0f", nextTurnDistanceM));
                            }
                            if (turnZoomActive) {
                                if (nextTurnDistanceM < 300) {
                                    targetMag = Math.max(targetMag, 16.0);
                                } else {
                                    targetMag = Math.max(targetMag, 15.0);
                                }
                            }

                            int currentBand = findBand(speed);
                            if (autoZoomSuspended) {
                                // Re-engage if speed crossed a threshold boundary
                                if (currentBand != lastAutoZoomBand) {
                                    autoZoomSuspended = false;
                                }
                            }
                            if (!autoZoomSuspended) {
                                // Smooth zoom: move toward target at 1 level per second
                                // (position updates arrive ~1/sec, so 1 level per update)
                                double step = 1.0;
                                if (targetMag > mag) {
                                    mag = Math.min(mag + step, targetMag);
                                } else if (targetMag < mag) {
                                    mag = Math.max(mag - step, targetMag);
                                }
                                currentSmoothMag = mag;
                                if (Math.abs(mag - targetMag) < 0.01) {
                                    lastAutoZoomBand = currentBand;
                                } else if (Math.abs(mag - targetMag) > 0.5) {
                                    // Log when still converging (more than 0.5 level away)
                                    Log.error("[AutoZoom] speed=" + String.format("%.1f", speed)
                                        + " cur=" + String.format("%.1f", mag)
                                        + " tgt=" + String.format("%.1f", targetMag)
                                        + " turnDist=" + String.format("%.0f", nextTurnDistanceM));
                                }
                            }
                        }
                        renderer.requestRenderPreserveRoute(position.lat, position.lon,
                            (int) Math.round(mag), angle);
                    }
                    if (routePanel != null) {
                        routePanel.updateNavigationStatus(position);
                    }
                    // Throttled road info lookup from current geo coordinate
                    updateRoadInfoFromPosition(position.lat, position.lon);
                });
            }

            @Override
            public void onRerouteRequest(double lat, double lon, double bearing, double destLat, double destLon) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    handleRerouteRequest(lat, lon, bearing, destLat, destLon);
                });
            }

            @Override
            public void onTargetReached(double bearing, double distance) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    stopNavigation();
                });
            }

            @Override
            public void onArrivalEstimate(long arrivalEstimate, double remainingDistance) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (routePanel != null) {
                        routePanel.updateArrivalEstimate(arrivalEstimate, remainingDistance);
                    }
                });
            }

            @Override
            public void onCurrentSpeed(double speedKmH) {
                if (isStale()) return;
                lastSpeedKmH = speedKmH;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (routePanel != null) {
                        routePanel.updateCurrentSpeed(speedKmH);
                    }
                });
            }

            @Override
            public void onMaxAllowedSpeed(double maxSpeedKmH) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (routePanel != null) {
                        routePanel.updateMaxAllowedSpeed(maxSpeedKmH);
                    }
                });
            }

            @Override
            public void onLaneUpdate(boolean oneway, int count, boolean suggested,
                                     int suggestedFrom, int suggestedTo, String turn,
                                     LaneTurn[] turns) {
                if (isStale()) return;
                Log.info("[Navigation] laneUpdate: count=" + count
                    + " suggested=" + suggested
                    + " from=" + suggestedFrom + " to=" + suggestedTo
                    + " turn=" + turn
                    + " turns=" + (turns != null ? turns.length : 0));
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (routePanel != null) {
                        routePanel.updateLaneInfo(oneway, count, suggested, suggestedFrom, suggestedTo, turn, turns);
                    }
                    updateNextTurnLanes(oneway, count, suggested, suggestedFrom, suggestedTo, turns);
                });
            }

            @Override
            public void onError(String message) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    Log.error("[MainController] navigation error: " + message);
                });
            }

            @Override
            public void onNextRouteInstruction(RouteInstruction instruction) {
                if (isStale()) return;
                double dist = instruction != null ? instruction.distanceTo : Double.POSITIVE_INFINITY;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    nextTurnDistanceM = dist;
                    updateNextTurnOverlay(instruction);
                });
            }

            @Override
            public void onRouteInstructions(RouteInstruction[] instructions) {
                if (isStale()) return;
                Platform.runLater(() -> {
                    if (isStale()) return;
                    if (routePanel != null) {
                        routePanel.updateInstructionList(instructions);
                    }
                });
            }
        };
    }

    // ---- Reroute handling ----

    private void handleRerouteRequest(double lat, double lon, double bearing,
                                       double destLat, double destLon) {
        if (client == null || routePanel == null || navigationController == null) {
            return;
        }

        // Cooldown check: ignore if we rerouted recently
        long now = System.currentTimeMillis();
        if (now - lastRerouteTime < REROUTE_COOLDOWN_MS) {
            return;
        }

        // Don't reroute if already in progress
        if (rerouting) {
            return;
        }

        rerouting = true;
        lastRerouteTime = now;
        rerouteGeneration++;

        // Show reroute status in route panel
        if (routePanel != null) {
            routePanel.setRerouteStatus(true, false);
        }

        // Cancel old navigation but keep route overlay visible
        if (navigationController != null) {
            navigationController.stop();
            navigationController = null;
        }
        // Pause track player instead of stopping it — preserves position
        // for resuming after reroute completes.
        if (trackPlayer != null) {
            trackPlayer.pause();
        }
        if (renderer != null) {
            renderer.clearCurrentLocation();
        }

        // Calculate new route from current position to original destination
        client.calculateRouteAsync(
            lat, lon,
            routeDestLat, routeDestLon,
            routePanel.getRoutingProfile(),
            createRerouteCallback());
    }

    private RouteCallback createRerouteCallback() {
        return new RouteCallback() {
            @Override
            public void onProgress(int percent) {
                // Reroute progress — could update the status label
            }

            @Override
            public void onSuccess(RouteEntry route) {
                Platform.runLater(() -> {
                    if (!rerouting) {
                        return; // was cancelled
                    }
                    rerouting = false;

                    // Update route overlay
                    if (renderer != null) {
                        renderer.requestRender(
                            renderer.getLatitude(),
                            renderer.getLongitude(),
                            renderer.getMagnification(),
                            route.latitudes,
                            route.longitudes);
                    }

                    // Update route panel with new route
                    if (routePanel != null) {
                        routePanel.setRerouteStatus(false, false);
                        routePanel.updateRerouteResult(route);
                    }

                    // Store new destination coords (last point of new route)
                    if (route.latitudes != null && route.latitudes.length > 0) {
                        int last = route.latitudes.length - 1;
                        routeDestLat = route.latitudes[last];
                        routeDestLon = route.longitudes[last];
                    }

                    // Start new navigation session
                    if (route.routeHandle != 0) {
                        navigationController = client.startNavigation(
                            route.routeHandle,
                            routePanel.getVehicle(),
                            createNavigationListener());
                        if (navigationController != null) {
                            followMode = true;
                            autoZoomEnabled = true;
                            autoZoomSuspended = false;
                            lastAutoZoomBand = -1;
                            updateFollowButtonStyle();
                            routePanel.setNavigationActive(true);
                            if (nextTurnBox != null) {
                                nextTurnBox.setVisible(true);
                                nextTurnBox.setManaged(true);
                            }
                            // Point paused track player at new controller and resume
                            if (trackPlayer != null) {
                                trackPlayer.setController(navigationController);
                                trackPlayer.play();
                            }
                        }
                    }
                });
            }

            @Override
            public void onError(String message) {
                Platform.runLater(() -> {
                    if (!rerouting) {
                        return; // was cancelled
                    }
                    rerouting = false;
                    Log.error("[MainController] reroute failed: " + message);
                    if (routePanel != null) {
                        routePanel.setRerouteStatus(false, true);
                    }
                });
            }

            @Override
            public void onCancel() {
                Platform.runLater(() -> {
                    rerouting = false;
                    if (routePanel != null) {
                        routePanel.setRerouteStatus(false, false);
                    }
                });
            }
        };
    }

    // ---- Long-press handler ----

    private void onLongPress(double lat, double lon) {
        longPressLat = lat;
        longPressLon = lon;

        if (client == null) {
            return;
        }

        Task<ObjectDescription> descTask = new Task<>() {
            @Override
            protected ObjectDescription call() {
                return client.getDescription(lat, lon);
            }
        };

        descTask.setOnSucceeded(e -> {
            ObjectDescription desc = descTask.getValue();
            if (desc == null || desc.getEntries().isEmpty()) {
                return;
            }
            showDescriptionOverlay(desc);
        });

        descTask.setOnFailed(e -> {
            Throwable err = descTask.getException();
            Log.error("[MainController] Description lookup failed: " +
                (err != null ? err.getMessage() : "unknown"));
            if (err != null) err.printStackTrace();
        });

        Thread thread = new Thread(descTask, "description-lookup");
        thread.setDaemon(true);
        thread.start();
    }

    private void showDescriptionOverlay(ObjectDescription desc) {
        descriptionOverlay = new DescriptionOverlay(uiScale, desc);
        mapPanel.getChildren().add(descriptionOverlay);
        descriptionOverlay.open();
    }

    // ---- Favorites dialog ----

    private void openFavoritesDialog() {
        if (client == null) return;
        Stage stage = (Stage) mapPanel.getScene().getWindow();
        String favPath = config.getFavoritesFilePath();
        FavLocationDialog dialog = new FavLocationDialog(
            stage, client, favPath, uiScale, this::onFavoriteJumpTo, this::updateFavoriteMarkers);
        dialog.show();
    }

    private void onFavoriteJumpTo(LocationEntry entry) {
        if (renderer != null) {
            renderer.requestRenderPreserveRoute(
                entry.lat, entry.lon, 16);
        }
    }

    // ---- Map position persistence ----

    private void restoreMapPosition() {
        String latStr = config.getMapLatitude();
        String lonStr = config.getMapLongitude();
        String magStr = config.getMapMagnification();

        if (latStr != null && lonStr != null && magStr != null) {
            try {
                double lat = Double.parseDouble(latStr);
                double lon = Double.parseDouble(lonStr);
                int mag = Integer.parseInt(magStr);
                if (renderer != null) {
                    renderer.requestRenderPreserveRoute(lat, lon, mag);
                }
            } catch (NumberFormatException ignored) {
            }
        }
    }

    private void saveMapPosition() {
        if (renderer != null && config != null) {
            config.setMapPosition(
                renderer.getLatitude(),
                renderer.getLongitude(),
                renderer.getMagnification());
        }
    }

    // ---- Public setters (called from JavaScoutApp) ----

    public void setUiScale(UIScale uiScale) {
        this.uiScale = uiScale;
    }

    public void setStylesheetDirectory(String stylesheetDirectory) {
        this.stylesheetDirectory = stylesheetDirectory;
    }

    public void setIconDirectory(String iconDirectory) {
        this.iconDirectory = iconDirectory;
        tryInit();
    }

    /**
     * Start initialisation once stylesheet/icon directories are set.
     * The maps directory is always the default download directory.
     * Guarded by initStarted to prevent double invocation.
     */
    private synchronized void tryInit() {
        if (initialised || initStarted) return;
        databaseDirectory = Config.getConfigDir().resolve("maps").toString();
        initStarted = true;
        initClientAndRenderer();
    }

    // ---- Map Download ----

    /**
     * Set the Config instance for persisting settings.
     */
    public void setConfig(Config config) {
        this.config = config;
    }

    @FXML
    private void onDownloadMaps() {
        Log.info("[MapDownload] onDownloadMaps called");
        if (client == null) {
            Log.info("[MapDownload] client is null, showing warning");
            Alert alert = new Alert(Alert.AlertType.WARNING,
                    "Client not initialised. Configure a maps directory first.",
                    javafx.scene.control.ButtonType.OK);
            alert.showAndWait();
            return;
        }
        Log.info("[MapDownload] client OK, loading FXML");

        try {
            FXMLLoader loader = new FXMLLoader(
                    getClass().getResource("/com/framstag/libosmscout/MapDownloadDialog.fxml"));
            Log.info("[MapDownload] FXML loaded, loading DialogPane");
            DialogPane dialogPane = loader.load();
            Log.info("[MapDownload] DialogPane loaded, getting controller");

            MapDownloadController controller = loader.getController();
            Log.info("[MapDownload] Controller: " + controller);
            controller.setClient(client);
            Log.info("[MapDownload] Client set on controller");
            controller.setConfig(config);
            Log.info("[MapDownload] Config set on controller");

            Dialog<ButtonType> dialog = new Dialog<>();
            dialog.setDialogPane(dialogPane);
            dialog.setTitle("Map Downloader");
            dialog.initOwner(mapCanvas.getScene().getWindow());
            // Add close button
            dialogPane.getButtonTypes().add(ButtonType.CLOSE);
            Log.info("[MapDownload] Showing dialog");
            dialog.showAndWait();
            Log.info("[MapDownload] Dialog closed");
        } catch (Exception e) {
            Log.error("[MapDownload] Error opening dialog: " + e.getMessage());
            e.printStackTrace();
            Alert alert = new Alert(Alert.AlertType.ERROR,
                    "Failed to open map download dialog: " + e.getMessage(),
                    javafx.scene.control.ButtonType.OK);
            alert.showAndWait();
        }
    }

    @FXML
    private void onQuit() {
        Stage stage = (Stage) mapCanvas.getScene().getWindow();
        stage.close();
    }

    // ---- Shutdown ----

    public void shutdown() {
        saveMapPosition();
        stopNavigation();
        if (routePanel != null) {
            routePanel.cancelRouting();
        }
        if (renderer != null) {
            renderer.shutdown();
        }
        if (client != null) {
            client.close();
        }
    }

    /**
     * Check whether {@code node} is the given parent or a descendant of it.
     * Used to decide whether a mouse event happened inside the menu button.
     */
    private static boolean isDescendantOf(javafx.scene.Parent parent, javafx.scene.Node node) {
        if (node == null || parent == null) {
            return false;
        }
        if (node == parent) {
            return true;
        }
        javafx.scene.Node p = node.getParent();
        while (p != null) {
            if (p == parent) {
                return true;
            }
            p = p.getParent();
        }
        return false;
    }
}
