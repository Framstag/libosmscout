package com.framstag.libosmscout;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.scene.layout.BorderPane;
import javafx.stage.Stage;

import javafx.geometry.Rectangle2D;

/**
 * JavaFX application entry point for JavaScout.
 * <p>
 * Loads the FXML layout, sets up the stage, and passes
 * stylesheet/icon directories to the controller.
 */
public class JavaScoutApp extends Application {

    /** Stylesheet directory passed from {@link JavaScout#main(String[])}. */
    static String stylesheetDirectory;

    /** Icon directory passed from {@link JavaScout#main(String[])}. */
    static String iconDirectory;

    /** Config instance passed from {@link JavaScout#main(String[])}. */
    static Config appConfig;

    @Override
    public void start(Stage stage) throws Exception {
        FXMLLoader loader = new FXMLLoader(
                getClass().getResource("/com/framstag/libosmscout/main.fxml"));

        BorderPane root = loader.load();

        UIScale uiScale = new UIScale();

        MainController controller = loader.getController();
        controller.setUiScale(uiScale);
        controller.setStylesheetDirectory(stylesheetDirectory);

        // Prefer system property for icon directory; fall back to static field for tests/legacy
        String resolvedIconDirectory = System.getProperty("javascout.iconDirectory", iconDirectory);
        controller.setIconDirectory(resolvedIconDirectory);

        Log.info("[JavaScoutApp] passed icon directory to controller: " + resolvedIconDirectory);

        if (appConfig != null) {
            controller.setConfig(appConfig);
        }

        Scene scene = new Scene(root, 800, 600);
        stage.setTitle("JavaScout");
        stage.setMinWidth(800);
        stage.setMinHeight(600);
        stage.setScene(scene);
        stage.show();

        // Clean up native resources on close
        stage.setOnCloseRequest(event -> controller.shutdown());
    }
}
