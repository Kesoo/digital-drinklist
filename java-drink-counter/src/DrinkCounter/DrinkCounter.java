package DrinkCounter;

import java.io.*;
import java.util.HashMap;
import java.util.Map;

public class DrinkCounter {

    private final Map<String,Integer> combinedDrinks;

    public DrinkCounter(){
        combinedDrinks = new HashMap<>();
    }

    public void countDrinks(File drinkFile){
        System.out.println("COUNT DRINKS");
        try {
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

        } catch (IOException e) {
            e.printStackTrace();
        }

        createOutputFile(combinedDrinks);
    }

    private void createOutputFile(Map<String,Integer> combinedDrinks) {

    }
}
