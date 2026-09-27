package com.framstag.libosmscout;

import com.framstag.libosmscout.client.FavoriteLocation;
import com.framstag.libosmscout.client.FavoriteLocationGroup;
import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.StarredFavoriteLocation;

import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Optional;
import java.util.function.Consumer;

/**
 * Modal dialog for managing favorite location groups and favorites.
 * <p>
 * Displays a list of groups on the left and the selected group's
 * favorites in the right. Supports add/delete for groups and
 * add/delete/rename for favorites. Double-click a favorite to jump
 * to it on the map.
 * <p>
 * Both lists are shown in the order the store reports, which is the order the
 * user chooses: the group list can be reordered, a favorite can be moved into
 * another group, and the third panel lists the starred favorites in their
 * cross-group order and lets the user reorder them.
 * <p>
 * When the loaded favorites file was written by a newer client, the dialog says
 * so instead of showing an empty list, and refuses to save over that file.
 */
public class FavLocationDialog extends Stage {

    private final OSMScoutClient client;
    private final String favFilePath;
    private final UIScale uiScale;
    private final Consumer<LocationEntry> onJumpTo;
    private final Runnable onFavoritesChanged;

    private final ListView<FavoriteLocationGroup> groupList;
    private final ObservableList<FavoriteLocationGroup> groups;
    private final ListView<FavoriteLocation> favList;
    private final ObservableList<FavoriteLocation> favorites;
    private final ListView<StarredFavoriteLocation> starredList;
    private final ObservableList<StarredFavoriteLocation> starred;

    private final Button moveGroupUpBtn;
    private final Button moveGroupDownBtn;
    private final Button moveStarredUpBtn;
    private final Button moveStarredDownBtn;
    private final Button saveBtn;

    /** False when the loaded file carries a version this client must not write over. */
    private final boolean fileVersionSupported;

    private boolean changed = false;

    /**
     * Create the favorite location management dialog.
     *
     * @param owner       parent stage
     * @param client      the OSMScoutClient for fav operations
     * @param favFilePath path to the favorites JSON file
     * @param uiScale     DPI-aware UI scale helper
     * @param onJumpTo    callback invoked on double-click to navigate to a favorite
     * @param onFavoritesChanged callback invoked when favorites are saved, may be null
     */
    public FavLocationDialog(Stage owner, OSMScoutClient client, String favFilePath, UIScale uiScale,
                             Consumer<LocationEntry> onJumpTo,
                             Runnable onFavoritesChanged) {
        this.client = client;
        this.favFilePath = favFilePath;
        this.uiScale = uiScale;
        this.onJumpTo = onJumpTo;
        this.onFavoritesChanged = onFavoritesChanged;
        this.fileVersionSupported = client.isFavoriteFileFormatSupported();

        initOwner(owner);
        initModality(Modality.APPLICATION_MODAL);
        setTitle("Manage Favorite Locations");

        double controlHeight = uiScale.controlHeight();
        double baseFont = uiScale.baseFontSize();

        // --- Groups panel (left) ---
        Label groupLabel = new Label("Groups");
        groupLabel.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold;");

        groups = FXCollections.observableArrayList();
        favorites = FXCollections.observableArrayList();
        starred = FXCollections.observableArrayList();
        groupList = new ListView<>(groups);
        groupList.setPrefWidth(uiScale.px(200));
        groupList.setMinWidth(uiScale.px(150));
        groupList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(FavoriteLocationGroup item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else {
                    String colorStr = item.attributes.get("color");
                    if (colorStr != null && colorStr.length() == 6) {
                        Rectangle swatch = new Rectangle(12, 12);
                        try {
                            swatch.setFill(Color.web("#" + colorStr));
                        } catch (Exception e) {
                            swatch.setFill(Color.TRANSPARENT);
                        }
                        swatch.setStroke(Color.GRAY);
                        swatch.setStrokeWidth(0.5);
                        setGraphic(swatch);
                        setText("  " + item.name);
                    } else {
                        setGraphic(null);
                        setText(item.name);
                    }
                }
            }
        });
        groupList.getSelectionModel().selectedItemProperty().addListener((obs, oldVal, newVal) -> {
            if (newVal != null) {
                loadFavorites(newVal);
            } else {
                favorites.clear();
            }
            updateMoveButtons();
        });

        Button addGroupBtn = new Button("Add Group");
        addGroupBtn.setMinHeight(controlHeight);
        addGroupBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        addGroupBtn.setOnAction(e -> addGroup());

        Button deleteGroupBtn = new Button("Delete Group");
        deleteGroupBtn.setMinHeight(controlHeight);
        deleteGroupBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        deleteGroupBtn.setOnAction(e -> deleteGroup());

        Button colorGroupBtn = new Button("Color");
        colorGroupBtn.setMinHeight(controlHeight);
        colorGroupBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        colorGroupBtn.setOnAction(e -> pickGroupColor());

        // Group order: the list shows the stored order, so a group is moved one
        // position by moving it there and refreshing the list from the store.
        moveGroupUpBtn = new Button("\u25b2 Up");
        moveGroupUpBtn.setMinHeight(controlHeight);
        moveGroupUpBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        moveGroupUpBtn.setOnAction(e -> moveSelectedGroup(-1));

        moveGroupDownBtn = new Button("\u25bc Down");
        moveGroupDownBtn.setMinHeight(controlHeight);
        moveGroupDownBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        moveGroupDownBtn.setOnAction(e -> moveSelectedGroup(1));

        HBox groupButtons = new HBox(uiScale.px(4), addGroupBtn, deleteGroupBtn, colorGroupBtn,
                moveGroupUpBtn, moveGroupDownBtn);
        groupButtons.setAlignment(Pos.CENTER_LEFT);

        VBox groupPanel = new VBox(uiScale.px(4), groupLabel, groupList, groupButtons);
        VBox.setVgrow(groupList, Priority.ALWAYS);

        // --- Favorites panel (right) ---
        Label favLabel = new Label("Favorites");
        favLabel.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold;");

        favList = new ListView<>(favorites);
        favList.setPrefWidth(uiScale.px(300));
        favList.setMinWidth(uiScale.px(200));
        favList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(FavoriteLocation item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                } else {
                    String star = "true".equals(item.attributes.get("starred")) ? "★ " : "";
                    setText(star + item.name + "  (" + String.format(Locale.US, "%.4f", item.lat) + ", " + String.format(Locale.US, "%.4f", item.lon) + ")");
                }
            }
        });

        favList.setOnMouseClicked(e -> {
            if (e.getClickCount() == 2) {
                FavoriteLocation selected = favList.getSelectionModel().getSelectedItem();
                if (selected != null) {
                    jumpToFavorite(selected);
                }
            }
        });

        Button addFavBtn = new Button("Add Favorite");
        addFavBtn.setMinHeight(controlHeight);
        addFavBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        addFavBtn.setOnAction(e -> addFavorite());

        Button deleteFavBtn = new Button("Delete");
        deleteFavBtn.setMinHeight(controlHeight);
        deleteFavBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        deleteFavBtn.setOnAction(e -> deleteFavorite());

        Button renameFavBtn = new Button("Rename");
        renameFavBtn.setMinHeight(controlHeight);
        renameFavBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        renameFavBtn.setOnAction(e -> renameFavorite());

        Button starFavBtn = new Button("Star");
        starFavBtn.setMinHeight(controlHeight);
        starFavBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        starFavBtn.setOnAction(e -> toggleStar());

        Button moveFavToGroupBtn = new Button("Move To...");
        moveFavToGroupBtn.setMinHeight(controlHeight);
        moveFavToGroupBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        moveFavToGroupBtn.setOnAction(e -> moveSelectedFavoriteToGroup());

        HBox favButtons = new HBox(uiScale.px(4), addFavBtn, deleteFavBtn, renameFavBtn, starFavBtn,
                moveFavToGroupBtn);
        favButtons.setAlignment(Pos.CENTER_LEFT);

        VBox favPanel = new VBox(uiScale.px(4), favLabel, favList, favButtons);
        VBox.setVgrow(favList, Priority.ALWAYS);

        // --- Starred panel (right) ---
        // One order across all groups, so each entry names the group it lives in.
        Label starredLabel = new Label("Starred");
        starredLabel.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold;");

        starredList = new ListView<>(starred);
        starredList.setPrefWidth(uiScale.px(240));
        starredList.setMinWidth(uiScale.px(160));
        starredList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(StarredFavoriteLocation item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                } else {
                    setText("\u2605 " + item.favorite.name + "  [" + item.groupName + "]");
                }
            }
        });
        starredList.getSelectionModel().selectedItemProperty().addListener(
                (obs, oldVal, newVal) -> updateMoveButtons());

        moveStarredUpBtn = new Button("\u25b2 Up");
        moveStarredUpBtn.setMinHeight(controlHeight);
        moveStarredUpBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        moveStarredUpBtn.setOnAction(e -> moveSelectedStarred(-1));

        moveStarredDownBtn = new Button("\u25bc Down");
        moveStarredDownBtn.setMinHeight(controlHeight);
        moveStarredDownBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        moveStarredDownBtn.setOnAction(e -> moveSelectedStarred(1));

        HBox starredButtons = new HBox(uiScale.px(4), moveStarredUpBtn, moveStarredDownBtn);
        starredButtons.setAlignment(Pos.CENTER_LEFT);

        VBox starredPanel = new VBox(uiScale.px(4), starredLabel, starredList, starredButtons);
        VBox.setVgrow(starredList, Priority.ALWAYS);

        // --- Main layout ---
        HBox mainPanel = new HBox(uiScale.px(8), groupPanel, favPanel, starredPanel);
        mainPanel.setPadding(new Insets(uiScale.px(8)));
        mainPanel.setPrefSize(uiScale.px(860), uiScale.px(440));

        // --- Bottom buttons ---
        saveBtn = new Button("Save");
        saveBtn.setMinHeight(controlHeight);
        saveBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        saveBtn.setDefaultButton(true);
        saveBtn.setDisable(!fileVersionSupported);
        saveBtn.setOnAction(e -> {
            if (saveChanges()) {
                close();
            }
        });

        Button cancelBtn = new Button("Cancel");
        cancelBtn.setMinHeight(controlHeight);
        cancelBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        cancelBtn.setCancelButton(true);
        cancelBtn.setOnAction(e -> close());

        HBox bottomBar = new HBox(uiScale.px(4), saveBtn, cancelBtn);
        bottomBar.setAlignment(Pos.CENTER_RIGHT);
        bottomBar.setPadding(new Insets(0, uiScale.px(8), uiScale.px(8), uiScale.px(8)));

        VBox root = new VBox(uiScale.px(4));

        // A file written by a newer client is reported instead of being shown as
        // an empty favorites list, and it is never written over.
        if (!fileVersionSupported) {
            Label notice = new Label("This favorites file was written by a newer version of "
                    + "this application (file format " + client.getFavoriteFileFormatVersion()
                    + "). It is left untouched and cannot be saved by this version.");
            notice.setWrapText(true);
            notice.setStyle("-fx-font-size: " + baseFont + "px; -fx-text-fill: #a04000;"
                    + " -fx-padding: " + uiScale.px(8) + "px;");
            root.getChildren().add(notice);
        }

        root.getChildren().addAll(mainPanel, bottomBar);
        VBox.setVgrow(mainPanel, Priority.ALWAYS);

        Scene scene = new Scene(root);
        setScene(scene);

        // Load existing data
        loadGroups();
    }

    private void loadGroups() {
        FavoriteLocationGroup[] loaded = client.getFavoriteGroups();
        groups.setAll(loaded != null ? loaded : new FavoriteLocationGroup[0]);
        loadStarred();
        updateMoveButtons();
    }

    /**
     * Reload the starred order from the store. The store owns the order, so the
     * list is always the store's order and never a locally kept one.
     */
    private void loadStarred() {
        StarredFavoriteLocation[] loaded = client.getStarredFavorites();
        starred.setAll(loaded != null ? loaded : new StarredFavoriteLocation[0]);
        updateMoveButtons();
    }

    /**
     * Reload the groups from the store and select the group with the given name,
     * which is how both group lists are refreshed after a favorite changed
     * groups: the store's view of both groups is the one that counts.
     *
     * @param selectGroupName group to select after the reload
     */
    private void reloadGroups(String selectGroupName) {
        loadGroups();

        for (FavoriteLocationGroup group : groups) {
            if (group.name.equals(selectGroupName)) {
                groupList.getSelectionModel().select(group);
                break;
            }
        }
    }

    /**
     * Enable a move action only where it can change something: something must be
     * selected, and the selection must not already sit at the border of its list.
     */
    private void updateMoveButtons() {
        int groupIndex = groupList.getSelectionModel().getSelectedIndex();
        moveGroupUpBtn.setDisable(!fileVersionSupported || groupIndex <= 0);
        moveGroupDownBtn.setDisable(!fileVersionSupported || groupIndex < 0
                || groupIndex >= groups.size() - 1);

        int starredIndex = starredList.getSelectionModel().getSelectedIndex();
        moveStarredUpBtn.setDisable(!fileVersionSupported || starredIndex <= 0);
        moveStarredDownBtn.setDisable(!fileVersionSupported || starredIndex < 0
                || starredIndex >= starred.size() - 1);
    }

    /**
     * Whether the dialog may change the store at all. A file written by a newer
     * client is left untouched, so the changing actions refuse with a message
     * instead of making changes the dialog could never persist.
     *
     * @return true if the loaded file may be changed
     */
    private boolean ensureWritable() {
        if (fileVersionSupported) {
            return true;
        }

        showError("This favorites file was written by a newer version of this application "
                + "(file format " + client.getFavoriteFileFormatVersion()
                + "). It is left untouched and cannot be changed by this version.");
        return false;
    }

    private void moveSelectedGroup(int delta) {
        if (!ensureWritable()) {
            return;
        }

        int index = groupList.getSelectionModel().getSelectedIndex();
        if (index < 0) {
            return;
        }

        int targetIndex = index + delta;
        if (targetIndex < 0 || targetIndex >= groups.size()) {
            return;
        }

        FavoriteLocationGroup selected = groups.get(index);
        if (!client.moveGroup(selected.name, targetIndex)) {
            showError("Could not move group '" + selected.name + "'.");
            return;
        }

        groups.remove(index);
        groups.add(targetIndex, selected);
        groupList.getSelectionModel().select(targetIndex);
        updateMoveButtons();
    }

    private void moveSelectedFavoriteToGroup() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocation selectedFav = favList.getSelectionModel().getSelectedItem();
        FavoriteLocationGroup selectedGroup = groupList.getSelectionModel().getSelectedItem();
        if (selectedFav == null || selectedGroup == null) {
            showError("Select a favorite first.");
            return;
        }

        List<String> candidates = new ArrayList<>();
        for (FavoriteLocationGroup group : groups) {
            candidates.add(group.name);
        }

        ChoiceDialog<String> dialog = new ChoiceDialog<>(selectedGroup.name, candidates);
        dialog.setTitle("Move Favorite");
        dialog.setHeaderText("Move '" + selectedFav.name + "' into another group");
        dialog.setContentText("Group:");

        Optional<String> result = dialog.showAndWait();
        if (result.isEmpty() || result.get().equals(selectedGroup.name)) {
            return;
        }

        String targetName = result.get();
        if (!client.moveFavoriteToGroup(selectedGroup.name, selectedFav.name, targetName, 0)) {
            showError("Could not move '" + selectedFav.name + "' to '" + targetName + "'."
                    + " The group may already contain a favorite of that name.");
            return;
        }

        // Both groups changed, so take both from the store again
        changed = true;
        reloadGroups(targetName);
    }

    private void moveSelectedStarred(int delta) {
        if (!ensureWritable()) {
            return;
        }

        int index = starredList.getSelectionModel().getSelectedIndex();
        if (index < 0) {
            return;
        }

        int targetIndex = index + delta;
        if (targetIndex < 0 || targetIndex >= starred.size()) {
            return;
        }

        StarredFavoriteLocation selected = starred.get(index);
        if (!client.moveStarredFavorite(selected.groupName, selected.favorite.name, targetIndex)) {
            showError("Could not move '" + selected.favorite.name + "' in the starred order.");
            return;
        }

        starred.remove(index);
        starred.add(targetIndex, selected);
        starredList.getSelectionModel().select(targetIndex);
        updateMoveButtons();
    }

    private void loadFavorites(FavoriteLocationGroup group) {
        favorites.setAll(group.favorites);
    }

    private void addGroup() {
        if (!ensureWritable()) {
            return;
        }

        TextInputDialog dialog = new TextInputDialog();
        dialog.setTitle("Add Group");
        dialog.setHeaderText("Enter group name:");
        dialog.setContentText("Name:");

        Optional<String> result = dialog.showAndWait();
        result.ifPresent(name -> {
            if (!name.trim().isEmpty()) {
                if (client.addGroup(name.trim())) {
                    FavoriteLocationGroup newGroup = new FavoriteLocationGroup(name.trim());
                    groups.add(newGroup);
                    groupList.getSelectionModel().select(newGroup);
                    changed = true;
                } else {
                    showError("Group '" + name.trim() + "' already exists.");
                }
            }
        });
    }

    private void deleteGroup() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocationGroup selected = groupList.getSelectionModel().getSelectedItem();
        if (selected == null) return;

        Alert confirm = new Alert(Alert.AlertType.CONFIRMATION,
                "Delete group '" + selected.name + "' and all its favorites?",
                ButtonType.YES, ButtonType.NO);
        confirm.setTitle("Delete Group");
        Optional<ButtonType> result = confirm.showAndWait();

        if (result.isPresent() && result.get() == ButtonType.YES) {
            if (client.deleteGroup(selected.name)) {
                groups.remove(selected);
                favorites.clear();
                changed = true;
            }
        }
    }

    private void addFavorite() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocationGroup selectedGroup = groupList.getSelectionModel().getSelectedItem();
        if (selectedGroup == null) {
            showError("Select a group first.");
            return;
        }

        Dialog<FavoriteLocation> dialog = new Dialog<>();
        dialog.setTitle("Add Favorite");
        dialog.setHeaderText("Add favorite to group '" + selectedGroup.name + "'");

        TextField nameField = new TextField();
        nameField.setPromptText("Favorite name");
        TextField latField = new TextField();
        latField.setPromptText("Latitude");
        TextField lonField = new TextField();
        lonField.setPromptText("Longitude");
        TextField queryField = new TextField();
        queryField.setPromptText("Search query (optional)");
        queryField.setEditable(false);

        Button searchBtn = new Button("Search location...");
        searchBtn.setOnAction(e -> {
            LocationEntry entry = searchLocationForFavorite();
            if (entry != null) {
                if (nameField.getText().trim().isEmpty()) {
                    nameField.setText(entry.label);
                }
                latField.setText(String.format(Locale.US, "%.6f", entry.lat));
                lonField.setText(String.format(Locale.US, "%.6f", entry.lon));
                queryField.setText(entry.label);
            }
        });

        VBox content = new VBox(uiScale.px(4), nameField, latField, lonField, searchBtn, queryField);
        dialog.getDialogPane().setContent(content);

        ButtonType addBtn = new ButtonType("Add", ButtonBar.ButtonData.OK_DONE);
        dialog.getDialogPane().getButtonTypes().addAll(addBtn, ButtonType.CANCEL);

        dialog.setResultConverter(btn -> {
            if (btn == addBtn) {
                try {
                    String name = nameField.getText().trim();
                    double lat = Double.parseDouble(latField.getText().trim());
                    double lon = Double.parseDouble(lonField.getText().trim());
                    if (name.isEmpty()) return null;
                    FavoriteLocation fav = new FavoriteLocation(name, lat, lon);
                    String query = queryField.getText().trim();
                    if (!query.isEmpty()) {
                        fav.attributes.put("query", query);
                    }
                    return fav;
                } catch (NumberFormatException e) {
                    showError("Invalid latitude or longitude.");
                }
            }
            return null;
        });

        Optional<FavoriteLocation> result = dialog.showAndWait();
        result.ifPresent(fav -> {
            if (client.addFavorite(selectedGroup.name, fav.name, fav.lat, fav.lon)) {
                selectedGroup.favorites.add(fav);
                if (groupList.getSelectionModel().getSelectedItem() == selectedGroup) {
                    favorites.add(fav);
                }
                changed = true;
            } else {
                showError("Favorite '" + fav.name + "' already exists in this group.");
            }
        });
    }

    /**
     * Open a small search dialog to pick a location for a favorite.
     *
     * @return selected LocationEntry, or null if cancelled/no result
     */
    private LocationEntry searchLocationForFavorite() {
        Dialog<LocationEntry> dialog = new Dialog<>();
        dialog.setTitle("Search Location");
        dialog.setHeaderText("Search for a location to use as favorite");

        TextField queryField = new TextField();
        queryField.setPromptText("Search...");
        ListView<LocationEntry> resultList = new ListView<>();
        resultList.setPrefHeight(uiScale.px(200));

        resultList.setCellFactory(lv -> new ListCell<>() {
            @Override
            protected void updateItem(LocationEntry item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                } else {
                    StringBuilder sb = new StringBuilder();
                    if (item.label != null && !item.label.isEmpty()) {
                        sb.append(item.label);
                    }
                    if (item.adminRegionHierarchy != null && !item.adminRegionHierarchy.isEmpty()) {
                        if (sb.length() > 0) sb.append(" → ");
                        sb.append(item.adminRegionHierarchy);
                    }
                    if (sb.length() == 0) {
                        sb.append(String.format(Locale.US, "%.6f, %.6f", item.lat, item.lon));
                    }
                    setText(sb.toString());
                }
            }
        });

        javafx.animation.PauseTransition debounce = new javafx.animation.PauseTransition(javafx.util.Duration.millis(400));
        debounce.setOnFinished(e -> {
            String query = queryField.getText().trim();
            if (query.isEmpty()) {
                return;
            }

            javafx.concurrent.Task<List<LocationEntry>> searchTask = new javafx.concurrent.Task<>() {
                @Override
                protected List<LocationEntry> call() {
                    LocationEntry[] results = client.searchLocations(query, 20, 0L);
                    return results != null ? List.of(results) : List.of();
                }
            };

            searchTask.setOnSucceeded(ev -> {
                resultList.getItems().setAll(searchTask.getValue());
                if (!resultList.getItems().isEmpty()) {
                    resultList.getSelectionModel().select(0);
                }
            });

            Thread thread = new Thread(searchTask, "fav-location-search");
            thread.setDaemon(true);
            thread.start();
        });

        queryField.textProperty().addListener((obs, oldVal, newVal) -> debounce.playFromStart());
        queryField.setOnAction(e -> debounce.playFromStart());

        VBox content = new VBox(uiScale.px(4), queryField, resultList);
        dialog.getDialogPane().setContent(content);
        dialog.getDialogPane().getButtonTypes().addAll(ButtonType.OK, ButtonType.CANCEL);

        dialog.setResultConverter(btn -> {
            if (btn == ButtonType.OK) {
                return resultList.getSelectionModel().getSelectedItem();
            }
            return null;
        });

        Optional<LocationEntry> result = dialog.showAndWait();
        return result.orElse(null);
    }

    private void deleteFavorite() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocation selected = favList.getSelectionModel().getSelectedItem();
        FavoriteLocationGroup selectedGroup = groupList.getSelectionModel().getSelectedItem();
        if (selected == null || selectedGroup == null) return;

        Alert confirm = new Alert(Alert.AlertType.CONFIRMATION,
                "Delete favorite '" + selected.name + "'?",
                ButtonType.YES, ButtonType.NO);
        confirm.setTitle("Delete Favorite");
        Optional<ButtonType> result = confirm.showAndWait();

        if (result.isPresent() && result.get() == ButtonType.YES) {
            if (client.deleteFavorite(selectedGroup.name, selected.name)) {
                selectedGroup.favorites.remove(selected);
                favorites.remove(selected);
                changed = true;
            }
        }
    }

    private void renameFavorite() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocation selected = favList.getSelectionModel().getSelectedItem();
        FavoriteLocationGroup selectedGroup = groupList.getSelectionModel().getSelectedItem();
        if (selected == null || selectedGroup == null) return;

        TextInputDialog dialog = new TextInputDialog(selected.name);
        dialog.setTitle("Rename Favorite");
        dialog.setHeaderText("Enter new name for '" + selected.name + "':");
        dialog.setContentText("Name:");

        Optional<String> result = dialog.showAndWait();
        result.ifPresent(newName -> {
            if (!newName.trim().isEmpty() && !newName.trim().equals(selected.name)) {
                if (client.renameFavorite(selectedGroup.name, selected.name, newName.trim())) {
                    selected.name = newName.trim();
                    favList.refresh();
                    changed = true;
                } else {
                    showError("Could not rename. Name '" + newName.trim() + "' may already exist.");
                }
            }
        });
    }

    private void toggleStar() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocation selected = favList.getSelectionModel().getSelectedItem();
        FavoriteLocationGroup selectedGroup = groupList.getSelectionModel().getSelectedItem();
        if (selected == null || selectedGroup == null) return;

        boolean currentlyStarred = "true".equals(selected.attributes.get("starred"));
        boolean newStarred = !currentlyStarred;

        if (client.setStarred(selectedGroup.name, selected.name, newStarred)) {
            if (newStarred) {
                selected.attributes.put("starred", "true");
            } else {
                selected.attributes.remove("starred");
            }
            favList.refresh();

            // Starring appends to the starred order and unstarring removes the
            // entry, so the starred list follows the store again
            loadStarred();
            changed = true;
        }
    }

    private void pickGroupColor() {
        if (!ensureWritable()) {
            return;
        }

        FavoriteLocationGroup selected = groupList.getSelectionModel().getSelectedItem();
        if (selected == null) return;

        ColorPicker colorPicker = new ColorPicker();
        String currentColor = selected.attributes.get("color");
        if (currentColor != null && currentColor.length() == 6) {
            try {
                colorPicker.setValue(Color.web("#" + currentColor));
            } catch (Exception e) {
                colorPicker.setValue(Color.TRANSPARENT);
            }
        } else {
            colorPicker.setValue(Color.TRANSPARENT);
        }

        Dialog<javafx.scene.paint.Color> dialog = new Dialog<>();
        dialog.setTitle("Pick Group Color");
        dialog.setHeaderText("Choose a color for group '" + selected.name + "'");

        Button clearBtn = new Button("Clear Color");
        clearBtn.setOnAction(e -> {
            dialog.setResult(Color.TRANSPARENT);
            dialog.close();
        });

        VBox content = new VBox(uiScale.px(8), colorPicker, clearBtn);
        dialog.getDialogPane().setContent(content);
        dialog.getDialogPane().getButtonTypes().addAll(ButtonType.OK, ButtonType.CANCEL);

        dialog.setResultConverter(btn -> {
            if (btn == ButtonType.OK) {
                return colorPicker.getValue();
            }
            return null;
        });

        Optional<javafx.scene.paint.Color> result = dialog.showAndWait();
        result.ifPresent(color -> {
            String hex;
            if (color.equals(Color.TRANSPARENT)) {
                hex = "";
            } else {
                hex = String.format("%02X%02X%02X",
                        (int)(color.getRed() * 255),
                        (int)(color.getGreen() * 255),
                        (int)(color.getBlue() * 255));
            }
            if (client.setGroupColor(selected.name, hex)) {
                if (hex.isEmpty()) {
                    selected.attributes.remove("color");
                } else {
                    selected.attributes.put("color", hex);
                }
                groupList.refresh();
                changed = true;
            }
        });
    }

    /**
     * Persist the current content of the dialog.
     *
     * @return true if the content is persisted (or nothing had to be), false when it could not be
     *         written, which includes a favorites file this client must not overwrite
     */
    private boolean saveChanges() {
        if (!fileVersionSupported) {
            showError("This favorites file was written by a newer version of this application "
                    + "(file format " + client.getFavoriteFileFormatVersion()
                    + "). It is left untouched and cannot be saved by this version.");
            return false;
        }

        if (!changed) {
            return true;
        }

        // Build array from current UI state
        FavoriteLocationGroup[] groupArray = groups.toArray(new FavoriteLocationGroup[0]);
        if (client.saveFavoriteLocations(favFilePath, groupArray)) {
            changed = false;
            if (onFavoritesChanged != null) {
                onFavoritesChanged.run();
            }
            return true;
        }

        showError("Saving the favorites file failed, so the changes were not written.");
        return false;
    }

    private void showError(String message) {
        Alert alert = new Alert(Alert.AlertType.ERROR, message, ButtonType.OK);
        alert.setTitle("Error");
        alert.showAndWait();
    }

    private void jumpToFavorite(FavoriteLocation fav) {
        if (onJumpTo == null) {
            return;
        }
        LocationEntry entry = new LocationEntry();
        entry.label = fav.name;
        entry.lat = fav.lat;
        entry.lon = fav.lon;
        entry.matchQuality = "favorite";
        if (!saveChanges()) {
            // The jump itself is harmless, but a failed save is reported rather
            // than swallowed; the dialog stays open when it could not persist
            return;
        }
        close();
        onJumpTo.accept(entry);
    }
}
