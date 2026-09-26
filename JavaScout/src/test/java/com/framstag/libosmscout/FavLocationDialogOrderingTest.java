package com.framstag.libosmscout;

import com.framstag.libosmscout.client.FavoriteLocation;
import com.framstag.libosmscout.client.FavoriteLocationGroup;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;
import com.framstag.libosmscout.client.StarredFavoriteLocation;

import javafx.application.Platform;
import javafx.scene.Node;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.control.TreeItem;
import javafx.scene.control.TreeView;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Drives the favorites dialog and checks that its ordering actions reach the
 * client: the group list follows the stored order and its up/down actions
 * reorder it, the starred list follows the store's starred order and its
 * up/down actions reorder that, and a file written by a newer client is
 * reported instead of shown as an empty list.
 * <p>
 * The dialog is built for real but never shown, so the tests need a working
 * JavaFX toolkit (run the module under {@code xvfb-run} where no display
 * exists). They are skipped when the toolkit or the native library is missing.
 */
public class FavLocationDialogOrderingTest {

    @TempDir
    Path tempDir;

    private OSMScoutClient client;
    private FavLocationDialog dialog;
    private String filePath;

    @BeforeAll
    public static void startToolkit() {
        try {
            FutureTask<Void> startup = new FutureTask<>(() -> null);
            Platform.startup(startup);
            startup.get();
        } catch (IllegalStateException e) {
            // Toolkit already started by another test class
        } catch (Throwable e) {
            Assumptions.assumeTrue(false,
                "JavaFX toolkit not available: " + e.getMessage());
        }
    }

    @BeforeEach
    public void setUp() {
        client = buildClient();
        if (client == null) {
            Assumptions.assumeTrue(false, "Native library not available");
        }

        filePath = tempDir.resolve("favorites.json").toString();
        assertTrue(client.loadFavoriteLocations(filePath));

        assertTrue(client.addGroup("Work"));
        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Uni"));
        assertTrue(client.addFavorite("Work", "Office", 51.5, 7.25));
        assertTrue(client.addFavorite("Work", "Desk", 51.4, 7.2));
        assertTrue(client.addFavorite("Home", "Flat", 51.3, 7.1));

        dialog = onFxThread(() -> new FavLocationDialog(null, client, filePath,
            new UIScale(), null, null));
        assertNotNull(dialog);
    }

    @AfterEach
    public void tearDown() {
        if (dialog != null) {
            onFxThread(() -> {
                dialog.close();
                return null;
            });
            dialog = null;
        }

        if (client != null) {
            client.close();
            client = null;
        }
    }

    private static OSMScoutClient buildClient() {
        try {
            return new OSMScoutClientBuilder()
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .build();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            return null;
        }
    }

    private static <T> T onFxThread(Callable<T> action) {
        FutureTask<T> task = new FutureTask<>(action);

        if (Platform.isFxApplicationThread()) {
            task.run();
        } else {
            Platform.runLater(task);
        }

        try {
            return task.get();
        } catch (InterruptedException | ExecutionException e) {
            throw new IllegalStateException(e);
        }
    }

    private static void collect(Node node, List<Node> result) {
        result.add(node);

        if (node instanceof Parent parent) {
            for (Node child : parent.getChildrenUnmodifiable()) {
                collect(child, result);
            }
        }
    }

    private static List<Node> nodes(Node root) {
        List<Node> result = new ArrayList<>();
        collect(root, result);
        return result;
    }

    private static Button button(Node root, String text) {
        for (Node node : nodes(root)) {
            if (node instanceof Button button && text.equals(button.getText())) {
                return button;
            }
        }

        throw new IllegalStateException("No button with text '" + text + "'");
    }

    /**
     * Find a button inside one panel. Both ordered lists carry a button with the
     * same label, so a lookup that is not scoped to a panel would find the wrong
     * one.
     *
     * @param container panel to search in
     * @param text      button label
     * @return the button
     */
    private static Button buttonIn(Node container, String text) {
        return button(container, text);
    }

    @SuppressWarnings("unchecked")
    private static <T> ListView<T> listOf(Node root, Class<T> itemType) {
        for (Node node : nodes(root)) {
            if (node instanceof ListView<?> view
                && !view.getItems().isEmpty()
                && itemType.isInstance(view.getItems().get(0))) {
                return (ListView<T>)view;
            }
        }

        throw new IllegalStateException("No list of " + itemType.getSimpleName() + " found");
    }

    private static List<String> groupNamesOf(ListView<FavoriteLocationGroup> list) {
        return list.getItems().stream().map(group -> group.name).toList();
    }

    private static List<String> starredNamesOf(ListView<StarredFavoriteLocation> list) {
        return list.getItems().stream()
            .map(entry -> entry.groupName + "/" + entry.favorite.name)
            .toList();
    }

    private static String groupNames(OSMScoutClient client) {
        StringBuilder result = new StringBuilder();

        for (FavoriteLocationGroup group : client.getFavoriteGroups()) {
            if (result.length() > 0) {
                result.append(',');
            }
            result.append(group.name);
        }

        return result.toString();
    }

    private static String starredNames(OSMScoutClient client) {
        StringBuilder result = new StringBuilder();

        for (StarredFavoriteLocation entry : client.getStarredFavorites()) {
            if (result.length() > 0) {
                result.append(',');
            }
            result.append(entry.groupName).append('/').append(entry.favorite.name);
        }

        return result.toString();
    }

    /**
     * Find the panel (the container of a list and its actions) by the caption of
     * its list. Used where a list is empty, so it cannot be identified by its
     * content.
     *
     * @param root      scene root to search in
     * @param labelText caption of the list
     * @return the panel container
     */
    private static Node panelWithLabel(Node root, String labelText) {
        for (Node node : nodes(root)) {
            if (node instanceof Label label && labelText.equals(label.getText())) {
                return label.getParent();
            }
        }

        throw new IllegalStateException("No panel with label '" + labelText + "'");
    }

    private void selectGroup(ListView<FavoriteLocationGroup> groups, String groupName) {
        for (FavoriteLocationGroup group : groups.getItems()) {
            if (group.name.equals(groupName)) {
                groups.getSelectionModel().select(group);
                return;
            }
        }
    }

    @Test
    public void theGroupListShowsTheStoredOrderAndItsActionsReorderIt() {
        onFxThread(() -> {
            Node root = dialog.getScene().getRoot();
            ListView<FavoriteLocationGroup> list = listOf(root, FavoriteLocationGroup.class);
            Button up = buttonIn(list.getParent(), "\u25b2 Up");
            Button down = buttonIn(list.getParent(), "\u25bc Down");

            assertEquals(List.of("Work", "Home", "Uni"), groupNamesOf(list),
                "the group list must show the stored order, not a sorted one");

            // At the borders the matching action is disabled
            list.getSelectionModel().select(0);
            assertTrue(up.isDisabled());
            assertFalse(down.isDisabled());

            list.getSelectionModel().select(2);
            assertFalse(up.isDisabled());
            assertTrue(down.isDisabled());

            // Moving the last group towards the beginning
            list.getSelectionModel().select(2);
            up.fire();

            assertEquals(List.of("Work", "Uni", "Home"), groupNamesOf(list));
            assertEquals("Work,Uni,Home", groupNames(client),
                "the move must have reached the store");

            // Moving the first group towards the end
            list.getSelectionModel().select(0);
            down.fire();

            assertEquals(List.of("Uni", "Work", "Home"), groupNamesOf(list));
            assertEquals("Uni,Work,Home", groupNames(client));

            // At the border the move action is disabled again: select the first
            // group, which cannot move towards the beginning
            list.getSelectionModel().select(0);
            assertTrue(buttonIn(list.getParent(), "\u25b2 Up").isDisabled());
            return null;
        });
    }

    @Test
    public void thereIsAMoveActionForTheSelectedFavoriteAndADialogReadsTheStore() {
        onFxThread(() -> {
            Node root = dialog.getScene().getRoot();
            ListView<FavoriteLocationGroup> groups = listOf(root, FavoriteLocationGroup.class);

            selectGroup(groups, "Work");

            ListView<FavoriteLocation> favs = listOf(root, FavoriteLocation.class);
            assertEquals(List.of("Office", "Desk"),
                favs.getItems().stream().map(fav -> fav.name).toList());

            // The action exists for a selected favorite (the choice dialog it
            // opens cannot be answered from a test, so its effect is asserted
            // through the client below)
            favs.getSelectionModel().select(1);
            assertFalse(button(root, "Move To...").isDisabled());
            return null;
        });

        // Move through the client, then open a fresh dialog: it must show the
        // store's new grouping rather than a copy it kept
        assertTrue(client.moveFavoriteToGroup("Work", "Desk", "Home", 0));

        FavLocationDialog reopened = onFxThread(() -> new FavLocationDialog(null, client,
            filePath, new UIScale(), null, null));

        try {
            onFxThread(() -> {
                Node root = reopened.getScene().getRoot();
                ListView<FavoriteLocationGroup> groups = listOf(root, FavoriteLocationGroup.class);

                selectGroup(groups, "Home");

                ListView<FavoriteLocation> favs = listOf(root, FavoriteLocation.class);
                assertEquals(List.of("Desk", "Flat"),
                    favs.getItems().stream().map(fav -> fav.name).toList());

                selectGroup(groups, "Work");
                assertEquals(List.of("Office"),
                    favs.getItems().stream().map(fav -> fav.name).toList());
                return null;
            });
        } finally {
            onFxThread(() -> {
                reopened.close();
                return null;
            });
        }
    }

    @Test
    public void theStarredListFollowsTheStoreOrderAndItsActionsReorderIt() {
        onFxThread(() -> {
            Node root = dialog.getScene().getRoot();
            ListView<FavoriteLocationGroup> groups = listOf(root, FavoriteLocationGroup.class);

            selectGroup(groups, "Work");

            ListView<FavoriteLocation> favs = listOf(root, FavoriteLocation.class);
            favs.getSelectionModel().select(0);
            button(root, "Star").fire();

            selectGroup(groups, "Home");
            favs.getSelectionModel().select(0);
            button(root, "Star").fire();

            assertEquals("Work/Office,Home/Flat", starredNames(client));
            return null;
        });

        FavLocationDialog reopened = onFxThread(() -> new FavLocationDialog(null, client,
            filePath, new UIScale(), null, null));

        try {
            onFxThread(() -> {
                Node root = reopened.getScene().getRoot();
                ListView<StarredFavoriteLocation> starred =
                    listOf(root, StarredFavoriteLocation.class);

                // Starring appended at the end, and each entry names its group
                assertEquals(List.of("Work/Office", "Home/Flat"), starredNamesOf(starred));

                // Moving the second entry towards the beginning
                starred.getSelectionModel().select(1);
                buttonIn(starred.getParent(), "\u25b2 Up").fire();

                assertEquals(List.of("Home/Flat", "Work/Office"), starredNamesOf(starred));
                assertEquals("Home/Flat,Work/Office", starredNames(client),
                    "the move must have reached the store");

                // At the border the action is disabled again
                starred.getSelectionModel().select(0);
                assertTrue(buttonIn(starred.getParent(), "\u25b2 Up").isDisabled());

                // Reordering stars did not reorder the groups
                ListView<FavoriteLocationGroup> groups = listOf(root, FavoriteLocationGroup.class);
                assertEquals(List.of("Work", "Home", "Uni"), groupNamesOf(groups));

                // Unstarring removes the entry from the list
                selectGroup(groups, "Home");

                ListView<FavoriteLocation> favs = listOf(root, FavoriteLocation.class);
                favs.getSelectionModel().select(0);
                button(root, "Star").fire();

                assertEquals(List.of("Work/Office"), starredNamesOf(starred));
                assertEquals("Work/Office", starredNames(client));
                return null;
            });
        } finally {
            onFxThread(() -> {
                reopened.close();
                return null;
            });
        }
    }

    @Test
    public void theFavoritePickerFollowsTheSameGroupOrder() {
        // Move a group so that the store order differs from the alphabetical one
        assertTrue(client.moveGroup("Uni", 0));

        FavLocationDialog reopened = onFxThread(() -> new FavLocationDialog(null, client,
            filePath, new UIScale(), null, null));
        FavoritePickerDialog picker = onFxThread(() -> new FavoritePickerDialog(null, client,
            new UIScale(), null));

        try {
            onFxThread(() -> {
                assertEquals(List.of("Uni", "Work", "Home"),
                    groupNamesOf(listOf(reopened.getScene().getRoot(), FavoriteLocationGroup.class)),
                    "the management dialog must show the stored order");

                TreeView<?> tree = null;
                for (Node node : nodes(picker.getScene().getRoot())) {
                    if (node instanceof TreeView<?> view) {
                        tree = view;
                    }
                }

                assertNotNull(tree, "the picker must show a tree");

                List<String> pickerGroups = new ArrayList<>();
                for (TreeItem<?> item : tree.getRoot().getChildren()) {
                    pickerGroups.add(((FavoriteLocationGroup)item.getValue()).name);
                }

                assertEquals(List.of("Uni", "Work", "Home"), pickerGroups,
                    "the picker must show the same group order as the dialog");
                return null;
            });
        } finally {
            onFxThread(() -> {
                picker.close();
                reopened.close();
                return null;
            });
        }
    }

    @Test
    public void anUnsupportedFileIsReportedAndCannotBeSaved() throws IOException {
        String content =
            "{\n" +
            "  \"formatVersion\": 2,\n" +
            "  \"groups\": [\n" +
            "    { \"name\": \"Work\", \"attributes\": {}, \"favorites\": [] }\n" +
            "  ]\n" +
            "}\n";
        Path newer = tempDir.resolve("newer.json");
        Files.writeString(newer, content);

        assertTrue(client.loadFavoriteLocations(newer.toString()));

        FavLocationDialog unsupported = onFxThread(() -> new FavLocationDialog(null, client,
            newer.toString(), new UIScale(), null, null));

        try {
            onFxThread(() -> {
                Node root = unsupported.getScene().getRoot();

                List<String> labels = new ArrayList<>();
                for (Node node : nodes(root)) {
                    if (node instanceof Label label && label.getText() != null) {
                        labels.add(label.getText());
                    }
                }

                // The version state is reported instead of an empty favorites list
                assertTrue(labels.stream().anyMatch(text -> text.contains("newer version")),
                    "the dialog must say the file was written by a newer version");
                assertTrue(labels.stream().anyMatch(text -> text.contains("file format 2")));

                // Saving over that file is not offered
                assertTrue(button(root, "Save").isDisabled(),
                    "the dialog must not offer to save over a newer file");

                // The ordering actions are refused as well
                assertTrue(button(panelWithLabel(root, "Groups"), "\u25b2 Up").isDisabled());
                return null;
            });
        } finally {
            onFxThread(() -> {
                unsupported.close();
                return null;
            });
        }

        // Opening and closing the dialog left the file alone
        assertEquals(content, Files.readString(newer));
    }
}
