package com.framstag.libosmscout;

import com.framstag.libosmscout.client.FavoriteLocation;
import com.framstag.libosmscout.client.FavoriteLocationGroup;
import com.framstag.libosmscout.client.LocationEntry;
import com.framstag.libosmscout.client.OSMScoutClient;

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

import java.util.Locale;
import java.util.function.Consumer;

/**
 * Modal dialog for selecting a favorite location from a tree of groups and favorites.
 */
public class FavoritePickerDialog extends Stage {

    private final OSMScoutClient client;
    private final UIScale uiScale;
    private final Consumer<LocationEntry> onSelect;

    private final TreeView<Object> favTree;

    public FavoritePickerDialog(Stage owner, OSMScoutClient client, UIScale uiScale,
                                Consumer<LocationEntry> onSelect) {
        this.client = client;
        this.uiScale = uiScale;
        this.onSelect = onSelect;

        initOwner(owner);
        initModality(Modality.APPLICATION_MODAL);
        setTitle("Select Favorite");

        double controlHeight = uiScale.controlHeight();
        double baseFont = uiScale.baseFontSize();

        Label title = new Label("Select a favorite location");
        title.setStyle("-fx-font-size: " + baseFont + "px; -fx-font-weight: bold;");

        favTree = new TreeView<>();
        favTree.setPrefHeight(uiScale.px(300));
        favTree.setMaxHeight(uiScale.px(450));
        VBox.setVgrow(favTree, Priority.ALWAYS);

        favTree.setCellFactory(tv -> new TreeCell<>() {
            @Override
            protected void updateItem(Object item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else if (item instanceof FavoriteLocationGroup group) {
                    String colorStr = group.attributes.get("color");
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
                        setText("  " + group.name + " (" + group.favorites.size() + ")");
                    } else {
                        setGraphic(null);
                        setText(group.name + " (" + group.favorites.size() + ")");
                    }
                } else if (item instanceof FavoriteLocation fav) {
                    String star = "true".equals(fav.attributes.get("starred")) ? "★ " : "";
                    setText(star + fav.name + "  (" + String.format(Locale.US, "%.4f", fav.lat) + ", " + String.format(Locale.US, "%.4f", fav.lon) + ")");
                }
            }
        });

        favTree.setOnMouseClicked(e -> {
            if (e.getClickCount() == 2) {
                selectCurrent();
            }
        });

        Button selectBtn = new Button("Select");
        selectBtn.setMinHeight(controlHeight);
        selectBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        selectBtn.setDefaultButton(true);
        selectBtn.setOnAction(e -> selectCurrent());

        Button cancelBtn = new Button("Cancel");
        cancelBtn.setMinHeight(controlHeight);
        cancelBtn.setStyle("-fx-font-size: " + baseFont + "px;");
        cancelBtn.setCancelButton(true);
        cancelBtn.setOnAction(e -> close());

        HBox buttons = new HBox(uiScale.px(4), selectBtn, cancelBtn);
        buttons.setAlignment(Pos.CENTER_RIGHT);

        VBox root = new VBox(uiScale.px(8), title, favTree, buttons);
        root.setPadding(new Insets(uiScale.px(8)));
        root.setPrefSize(uiScale.px(360), uiScale.px(400));

        Scene scene = new Scene(root);
        setScene(scene);

        loadFavorites();
    }

    private void loadFavorites() {
        FavoriteLocationGroup[] groups = client.getFavoriteGroups();

        TreeItem<Object> root = new TreeItem<>("Favorites");
        root.setExpanded(true);

        if (groups != null) {
            for (FavoriteLocationGroup group : groups) {
                TreeItem<Object> groupItem = new TreeItem<>(group);
                groupItem.setExpanded(true);
                for (FavoriteLocation fav : group.favorites) {
                    groupItem.getChildren().add(new TreeItem<>(fav));
                }
                root.getChildren().add(groupItem);
            }
        }

        favTree.setRoot(root);
        favTree.setShowRoot(false);
    }

    private void selectCurrent() {
        TreeItem<Object> selected = favTree.getSelectionModel().getSelectedItem();
        if (selected != null && selected.getValue() instanceof FavoriteLocation fav) {
            LocationEntry entry = new LocationEntry();
            entry.label = fav.name;
            entry.lat = fav.lat;
            entry.lon = fav.lon;
            entry.matchQuality = "favorite";

            String offsetStr = fav.attributes.get("objectFileOffset");
            if (offsetStr != null) {
                try {
                    entry.objectFileOffset = Long.parseLong(offsetStr);
                } catch (NumberFormatException ignored) {
                }
            }
            entry.refType = fav.attributes.get("refType");
            entry.objectType = fav.attributes.get("objectType");

            close();
            onSelect.accept(entry);
        }
    }
}
