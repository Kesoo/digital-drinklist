package DrinkCounter;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

public class Main extends Application {
    private static final String WINDOW_TITLE = "RUSTET Drink Counter";
    private static final int WINDOW_WIDTH  = 300;
    private static final int WINDOW_HEIGHT = 275;

    @Override
    public void start(Stage primaryStage) throws Exception {
        URL rootFXML = getClass().getResource("openFile.fxml");
        Parent root = FXMLLoader.load(rootFXML);
        primaryStage.setTitle(WINDOW_TITLE);
        primaryStage.setScene(new Scene(root, WINDOW_WIDTH, WINDOW_HEIGHT));
        primaryStage.show();
    }

    public static void main(String[] args) {
        launch(args);
    }
}
