package DrinkCounter;

import java.io.File;
import java.io.IOException;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.stage.FileChooser;

public class OpenFileController {
    @FXML
    public Button openFileButton;

    @FXML
    public Label responseLabel;

    private DrinkCounter drinkCounter;

    public OpenFileController() {
        this.drinkCounter = new DrinkCounter();
    }

    public void openFileAction(ActionEvent event){
        FileChooser fileChooser = new FileChooser();
        fileChooser.getExtensionFilters()
                   .add(new FileChooser.ExtensionFilter("TXT Files", "*.TXT"));

        File selectedFile = fileChooser.showOpenDialog(null);

        if (selectedFile == null) {
            responseLabel.setText("An error occurred: No file selected");
        } else {
            try {
                String outputFileName = drinkCounter.createDrinklist(selectedFile);
                responseLabel.setText(outputFileName + " created!");
            } catch (IOException exception) {
                // Handle exception
                responseLabel.setText("An error occured: " + exception.getMessage());
                exception.printStackTrace();
            }
        }
    }
}
