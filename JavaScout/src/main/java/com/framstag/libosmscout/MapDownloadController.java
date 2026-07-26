package com.framstag.libosmscout;

import com.framstag.libosmscout.client.AvailableMapEntry;
import com.framstag.libosmscout.client.MapDownloadListener;
import com.framstag.libosmscout.client.MapDownloadManager;
import com.framstag.libosmscout.client.MapProvider;
import com.framstag.libosmscout.client.OSMScoutClient;

import javafx.application.Platform;
import javafx.beans.property.SimpleStringProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.*;
import javafx.scene.control.cell.PropertyValueFactory;
import java.net.URL;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.ResourceBundle;

/**
 * Controller for the Map Download dialog.
 * <p>
 * Manages browsing available maps, downloading, and managing installed maps.
 */
public class MapDownloadController implements Initializable {

    @FXML
    private ComboBox<MapProvider> providerCombo;

    @FXML
    private Button refreshBtn;

    @FXML
    private TreeView<AvailableMapEntry> availableTree;

    @FXML
    private Label selectedMapLabel;

    @FXML
    private Button downloadBtn;

    @FXML
    private TableView<DownloadEntry> downloadsTable;

    @FXML
    private TableColumn<DownloadEntry, String> mapNameCol;

    @FXML
    private TableColumn<DownloadEntry, String> progressCol;

    @FXML
    private TableColumn<DownloadEntry, String> statusCol;

    @FXML
    private TableColumn<DownloadEntry, String> targetCol;

    @FXML
    private ListView<String> installedList;

    private OSMScoutClient client;
    private MapDownloadManager downloadManager;
    private Config config;
    private final ObservableList<DownloadEntry> downloads = FXCollections.observableArrayList();
    /** Map from displayed map name to full directory path for deletion. */
    private final java.util.Map<String, String> installedPaths = new java.util.HashMap<>();

    @Override
    public void initialize(URL location, ResourceBundle resources) {
        // Set up available maps tree
        availableTree.getSelectionModel().selectedItemProperty().addListener(
            (obs, old, selected) -> onTreeSelectionChanged(selected));
        availableTree.setCellFactory(tv -> new TreeCell<>() {
            @Override
            protected void updateItem(AvailableMapEntry item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                } else {
                    setText(item.getName());
                }
            }
        });

        // Set up downloads table
        mapNameCol.setCellValueFactory(new PropertyValueFactory<>("mapName"));
        progressCol.setCellValueFactory(new PropertyValueFactory<>("progress"));
        statusCol.setCellValueFactory(new PropertyValueFactory<>("status"));
        targetCol.setCellValueFactory(new PropertyValueFactory<>("targetDir"));
        downloadsTable.setItems(downloads);

        // Set up provider combo rendering
        providerCombo.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(MapProvider item, boolean empty) {
                super.updateItem(item, empty);
                setText(empty || item == null ? null : item.getName());
            }
        });
        providerCombo.setButtonCell(new ListCell<>() {
            @Override
            protected void updateItem(MapProvider item, boolean empty) {
                super.updateItem(item, empty);
                setText(empty || item == null ? null : item.getName());
            }
        });
    }

    /**
     * Set the OSMScoutClient and initialise providers.
     */
    public void setClient(OSMScoutClient client) {
        this.client = client;
        this.downloadManager = client.getMapDownloadManager();
        loadProviders();
        onRefreshInstalled();
    }

    /**
     * Set the Config for persisting provider selection.
     */
    public void setConfig(Config config) {
        this.config = config;
    }

    private void loadProviders() {
        // For now, add the default karry.cz provider
        // In a full implementation, providers would be loaded from a JSON resource
        MapProvider defaultProvider = new MapProvider(
            "karry.cz",
            "https://osmscout.karry.cz",
            "https://osmscout.karry.cz/latest.php?fromVersion=%1&toVersion=%2&locale=%3"
        );
        providerCombo.getItems().add(defaultProvider);
        providerCombo.getSelectionModel().select(0);

        // Restore saved provider
        if (config != null) {
            String saved = config.getMapProvider();
            if (saved != null) {
                for (MapProvider p : providerCombo.getItems()) {
                    if (p.getName().equals(saved)) {
                        providerCombo.getSelectionModel().select(p);
                        break;
                    }
                }
            }
        }
    }

    @FXML
    private void onRefresh() {
        MapProvider provider = providerCombo.getSelectionModel().getSelectedItem();
        if (provider == null || downloadManager == null) {
            return;
        }

        refreshBtn.setDisable(true);
        refreshBtn.setText("Loading...");

        new Thread(() -> {
            try {
                List<AvailableMapEntry> entries = downloadManager.fetchAvailableMaps(provider);
                Platform.runLater(() -> {
                    if (entries == null) {
                        showError("Failed to fetch map list: native method returned null");
                    } else {
                        buildTree(entries);
                    }
                    refreshBtn.setDisable(false);
                    refreshBtn.setText("Refresh");
                });
            } catch (Exception e) {
                Platform.runLater(() -> {
                    showError("Failed to fetch map list: " + e.getMessage());
                    refreshBtn.setDisable(false);
                    refreshBtn.setText("Refresh");
                });
            }
        }, "fetch-map-list").start();
    }

    private void buildTree(List<AvailableMapEntry> entries) {
        if (entries == null) {
            return;
        }

        // Build a map of path -> directory TreeItem
        java.util.Map<String, TreeItem<AvailableMapEntry>> dirNodes = new java.util.HashMap<>();
        TreeItem<AvailableMapEntry> root = new TreeItem<>(null);
        root.setExpanded(true);

        // First pass: collect all unique directory paths from entries
        // and create directory nodes for each path segment
        for (AvailableMapEntry entry : entries) {
            java.util.List<String> path = entry.getPath();
            if (path == null || path.isEmpty()) {
                // Top-level entry: add directly to root
                if (entry.isDirectory()) {
                    TreeItem<AvailableMapEntry> dirItem = new TreeItem<>(entry);
                    dirItem.setExpanded(false);
                    root.getChildren().add(dirItem);
                    dirNodes.put("", dirItem);
                } else {
                    root.getChildren().add(new TreeItem<>(entry));
                }
                continue;
            }

            // Build directory path string for lookup
            StringBuilder dirPath = new StringBuilder();
            for (int i = 0; i < path.size(); i++) {
                if (i > 0) dirPath.append("/");
                dirPath.append(path.get(i));

                String key = dirPath.toString();
                if (!dirNodes.containsKey(key)) {
                    // Create directory node for this path segment
                    String segName = path.get(i);
                    java.util.List<String> segPath = path.subList(0, i);
                    AvailableMapEntry dirEntry = new AvailableMapEntry(segName, segPath, "");
                    TreeItem<AvailableMapEntry> dirItem = new TreeItem<>(dirEntry);
                    dirItem.setExpanded(false);
                    dirNodes.put(key, dirItem);

                    // Attach to parent
                    if (i == 0) {
                        root.getChildren().add(dirItem);
                    } else {
                        String parentKey = dirPath.substring(0, dirPath.length() - segName.length() - 1);
                        TreeItem<AvailableMapEntry> parent = dirNodes.get(parentKey);
                        if (parent != null) {
                            parent.getChildren().add(dirItem);
                        }
                    }
                }
            }
        }

        // Second pass: insert map entries as leaf nodes
        for (AvailableMapEntry entry : entries) {
            if (entry.isDirectory()) {
                continue; // Directories already handled
            }
            java.util.List<String> path = entry.getPath();
            if (path == null || path.isEmpty()) {
                root.getChildren().add(new TreeItem<>(entry));
            } else {
                StringBuilder dirPath = new StringBuilder();
                for (int i = 0; i < path.size(); i++) {
                    if (i > 0) dirPath.append("/");
                    dirPath.append(path.get(i));
                }
                String key = dirPath.toString();
                TreeItem<AvailableMapEntry> parent = dirNodes.get(key);
                if (parent != null) {
                    parent.getChildren().add(new TreeItem<>(entry));
                } else {
                    root.getChildren().add(new TreeItem<>(entry));
                }
            }
        }

        availableTree.setRoot(root);
        availableTree.setShowRoot(false);
    }

    private void onTreeSelectionChanged(TreeItem<AvailableMapEntry> selected) {
        if (selected == null || selected.getValue() == null) {
            selectedMapLabel.setText("Select a map");
            downloadBtn.setDisable(true);
            return;
        }

        AvailableMapEntry entry = selected.getValue();
        if (entry.isDirectory()) {
            selectedMapLabel.setText(entry.getName() + " (directory)");
            downloadBtn.setDisable(true);
        } else {
            String sizeStr = entry.getSize() > 0
                ? String.format(" (%.1f MB)", entry.getSize() / (1024.0 * 1024.0))
                : "";
            selectedMapLabel.setText(entry.getName() + sizeStr);
            downloadBtn.setDisable(false);
        }
    }

    @FXML
    private void onDownload() {
        TreeItem<AvailableMapEntry> selected = availableTree.getSelectionModel().getSelectedItem();
        if (selected == null || selected.getValue() == null || selected.getValue().isDirectory()) {
            return;
        }

        AvailableMapEntry entry = selected.getValue();

        // Use default maps directory (~/.config/javascout/maps/) for downloads.
        // The CLI argument is only for lookup, not for storing new downloads.
        // Always use the default config dir for downloads, not the CLI override.
        java.nio.file.Path mapsPath = com.framstag.libosmscout.Config.getConfigDir().resolve("maps").toAbsolutePath();
        // Create a subdirectory named after the map
        Path targetDir = mapsPath
            .resolve(entry.getName().replaceAll("\\s+", "-").toLowerCase());

        // Add to downloads table
        DownloadEntry dlEntry = new DownloadEntry(entry.getName(), targetDir.toString());
        dlEntry.setStatus("Starting");
        downloads.add(dlEntry);

        // Start download
        String handle = downloadManager.downloadMap(entry, targetDir, new MapDownloadListener() {
            private long lastUiUpdate;

            @Override
            public void onProgress(String mapName, long bytesDownloaded, long totalBytes) {
                long now = System.currentTimeMillis();
                if (now - lastUiUpdate < 250) {
                    return;
                }
                lastUiUpdate = now;
                Platform.runLater(() -> {
                    dlEntry.setProgress(bytesDownloaded, totalBytes);
                });
            }

            @Override
            public void onComplete(String mapName, String targetDir) {
                Platform.runLater(() -> {
                    dlEntry.setComplete();
                    onRefreshInstalled();
                });
            }

            @Override
            public void onError(String mapName, String errorMessage) {
                Platform.runLater(() -> {
                    dlEntry.setError(errorMessage);
                });
            }
        });
        dlEntry.setHandle(handle);
    }

    @FXML
    private void onCancelDownload() {
        DownloadEntry selected = downloadsTable.getSelectionModel().getSelectedItem();
        if (selected != null && selected.getHandle() != null) {
            selected.setStatus("Cancelling...");
            downloadManager.cancelDownload(selected.getHandle());
        }
    }

    @FXML
    private void onDeleteMap() {
        String selected = installedList.getSelectionModel().getSelectedItem();
        if (selected != null && downloadManager != null) {
            String selectedPath = installedPaths.get(selected);
            if (selectedPath != null && downloadManager.deleteMap(selectedPath)) {
                installedList.getItems().remove(selected);
                installedPaths.remove(selected);
            } else {
                showError("Failed to delete map: " + selected);
            }
        }
    }

    @FXML
    private void onRefreshInstalled() {
        if (downloadManager == null) {
            return;
        }
        new Thread(() -> {
            List<String> dirs = downloadManager.getInstalledMaps();
            Platform.runLater(() -> {
                installedPaths.clear();
                List<String> names = new ArrayList<>();
                for (String dir : dirs) {
                    java.nio.file.Path p = java.nio.file.Paths.get(dir);
                    String name = p.getFileName() != null ? p.getFileName().toString() : dir;
                    installedPaths.put(name, dir);
                    names.add(name);
                }
                installedList.getItems().setAll(names);
            });
        }, "refresh-installed").start();
    }

    private void showError(String message) {
        Alert alert = new Alert(Alert.AlertType.ERROR, message, ButtonType.OK);
        alert.showAndWait();
    }

    // ---- Download entry for the table ----

    /**
     * Table entry for an active or completed download.
     */
    public static class DownloadEntry {
        private final SimpleStringProperty mapName;
        private final SimpleStringProperty targetDir;
        private final SimpleStringProperty progress = new SimpleStringProperty("0%");
        private final SimpleStringProperty status = new SimpleStringProperty("Queued");
        private String handle;

        public DownloadEntry(String mapName, String targetDir) {
            this.mapName = new SimpleStringProperty(mapName);
            this.targetDir = new SimpleStringProperty(targetDir);
        }

        public String getMapName() { return mapName.get(); }
        public javafx.beans.property.StringProperty mapNameProperty() { return mapName; }
        public String getTargetDir() { return targetDir.get(); }
        public javafx.beans.property.StringProperty targetDirProperty() { return targetDir; }
        public String getProgress() { return progress.get(); }
        public javafx.beans.property.StringProperty progressProperty() { return progress; }
        public String getStatus() { return status.get(); }
        public javafx.beans.property.StringProperty statusProperty() { return status; }

        public void setProgress(long downloaded, long total) {
            if (total > 0) {
                int pct = (int) (downloaded * 100 / total);
                if (pct >= 100) {
                    pct = 99;
                }
                progress.set(pct + "%");
            } else {
                progress.set(downloaded + " bytes");
            }
            if (!isTerminalStatus()) {
                status.set("Downloading");
            }
        }

        public void setComplete() {
            progress.set("100%");
            status.set("Complete");
        }

        public void setError(String message) {
            if ("Download cancelled".equals(message)) {
                status.set("Cancelled");
            } else {
                status.set("Error: " + message);
            }
        }

        private boolean isTerminalStatus() {
            String s = status.get();
            return "Complete".equals(s) || s.startsWith("Error:") || "Cancelling...".equals(s);
        }

        public void setStatus(String s) { status.set(s); }
        public String getHandle() { return handle; }
        public void setHandle(String h) { this.handle = h; }
    }
}
