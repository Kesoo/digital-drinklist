package DrinkCounter;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

public class DrinkCounter {

    private final Map<String,Integer> combinedDrinks;

    public DrinkCounter(){
        combinedDrinks = new HashMap<>();
    }

    public String createDrinklist(File drinkFile) throws IOException {
        Map<String, Integer> drinkMap = countDrinks(drinkFile);
        return createOutputFile(drinkMap);
    }

    private Map<String,Integer> countDrinks(File drinkFile) throws IOException{
        System.out.println("COUNT DRINKS");
        FileReader fileReader = new FileReader(drinkFile);
        BufferedReader bufferedReader = new BufferedReader(fileReader);

        String line;
        while ((line = bufferedReader.readLine()) != null) {
            line = line.trim();
            if (combinedDrinks.containsKey(line)) {
                combinedDrinks.replace(line, combinedDrinks.get(line)+1);
            } else {
                combinedDrinks.put(line, 1);
            }
        }
        fileReader.close();
        System.out.println(combinedDrinks);

        return combinedDrinks;
    }

    private String createOutputFile(Map<String,Integer> combinedDrinks) throws IOException{
        String now = LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd-HH-mm"));
        String fileName = now + "-strecklista.txt";
        System.out.println("CREATED: " + fileName);
        File outputFile = new File(fileName);

        boolean[] fileWriteSuccess = {true};
        boolean fileCreated = outputFile.createNewFile();

        if (fileCreated){

            FileWriter fileWriter = new FileWriter(outputFile);
            fileWriter.append("Strecklista genererad: ").append(now).append("\n");
            fileWriter.append("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

            combinedDrinks.entrySet()
                    .stream()
                    .sorted(Map.Entry.comparingByValue(Comparator.reverseOrder()))
                    .forEach(person -> {
                        try {
                            fileWriter.append(person.getKey()).append(": ").append(person.getValue().toString()).append("\n");
                        } catch (IOException e) {
                            fileWriteSuccess[0] = false;
                        }
                    });
            fileWriter.close();
        }

        if (fileCreated && fileWriteSuccess[0]) {
            return fileName;
        } else {
            throw new IOException(!fileCreated ? "File already exists" : "Error writing to file");
        }
    }
}
