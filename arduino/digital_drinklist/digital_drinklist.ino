#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>

#define SD_SS_PIN       4   // SD CARD
#define RST_PIN         9   // RFID
#define RFID_SS_PIN     8   // RFID
#define LED_RED         2
#define LED_GREEN       3
#define LED_BLUE        5
#define pulldowndelay   50
#define afterScanDelay  3000

MFRC522 mfrc522(RFID_SS_PIN, RST_PIN);  // Create MFRC522 (RFID) instance

String registerUsersId = "4920DBA3";

File users;
File drinkList;
File newUsers;

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  // LED SETUP
  pinMode(LED_RED,OUTPUT);
  pinMode(LED_GREEN,OUTPUT);
  pinMode(LED_BLUE,OUTPUT);
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_GREEN,LOW);
  digitalWrite(LED_BLUE,HIGH);

  // SPI SETUP
  SPI.begin();
  pinMode(SD_SS_PIN,OUTPUT);
  pinMode(RFID_SS_PIN,OUTPUT);
  noSPI();

  // RFID SETUP
  rfidSPI();
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks...")); 

  // SD CARD SETUP
  sdCardSPI();
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(SD_SS_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  rfidSPI();
}

void loop() {
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_GREEN,LOW);
  
  waitForRFIDCard();
  
  String currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
  currentUid.trim();

  if (registerUsersId.equalsIgnoreCase(currentUid)){
    registerUserMode();
    return;
  }

  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  
  sdCardSPI();
  Serial.println("Current Uid: " + currentUid);
  String userName = getNameFromUid(currentUid);
  if (userName != "NOUSER") {
    Serial.println("USER FOUND");
    registerDrink(userName);
  } else {
    Serial.println("NO USER FOUND"); 
  }

  rfidSPI();
  delay(afterScanDelay);  
}

void waitForRFIDCard(){
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  while(!mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial());
}

String getUid(byte *buffer, byte bufferSize) {
  String byteArray;
  for (byte i = 0; i < bufferSize; i++) {
    byteArray.concat(String(buffer[i], HEX));
    byteArray.toUpperCase();
  }
  return byteArray;
}

String getNameFromUid(String uid){
  users = SD.open("users.txt", FILE_READ);

  if (!users) {
    Serial.println("Error opening users.txt");
    return "NOUSER";  
  }
  String row = "";
  String userId = "";
  String userName = "NOUSER";
  String userNameToReturn;
  bool userIdInDB = false;
  while (users.available() != 0){
    row = users.readStringUntil('\n');
    if (row == ""){
      break;  
    }

    userId = getValue(row, ':', 0);
    if (uid.equalsIgnoreCase(userId)) {
      userName = getValue(row, ':', 1);
      userName.trim();
      if (userName.equalsIgnoreCase("")) {
        userName = "NOUSER";  
      }
    }
  }
  users.close();
  
  // Toggle LED's
  if (userName != "NOUSER") {
    digitalWrite(LED_RED,LOW);
  } else {
    digitalWrite(LED_GREEN,LOW);
  }

  return userName;
}

bool doesUserNeedRegistering(String uid){
  users = SD.open("users.txt", FILE_READ);

  if (!users) {
    Serial.println("Error opening users.txt");
    return false;  
  }
  String userId = "";
  bool userIdInDB = false;
  while (users.available() != 0){
    userId = users.readStringUntil('\n');
    if (userId == ""){
      break;  
    }
    userId.trim();
    if (uid.equalsIgnoreCase(userId)) {
      userIdInDB = true;
      break;
    }
  }
  users.close();

  if (userIdInDB) {
    return false;  
  }

  Serial.println("READING NEWUSERS");
  newUsers = SD.open("newusers.txt", FILE_READ);

  if (!newUsers) {
    Serial.println("Error opening newusers.txt");
    return false;  
  }
  String nUserId = "";
  bool userIdInNewUsers = false;
  while (newUsers.available() != 0){
    nUserId = newUsers.readStringUntil('\n');
    if (nUserId == ""){
      break;  
    }
    nUserId.trim();
    Serial.println("MATCHIN WITH: " + nUserId + "from " + uid);
    
    if (uid.equalsIgnoreCase(nUserId)) {
      Serial.println("MATCH");
      userIdInNewUsers = true;
      break;
    }
  }
  newUsers.close();
  
  return !userIdInNewUsers;
}

void registerDrink(String userName) {
  Serial.println("REGISTER DRINK: " + userName);
  drinkList = SD.open("drinks.txt", FILE_WRITE);
  userName.trim();
  // if the file opened okay, write to it:
  if (drinkList) {
    Serial.print("Writing to drinks.txt...");
    drinkList.println(userName);
    // close the file:
    drinkList.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening drinks.txt");
  }
}

void registerNewUser(String uid) {
  Serial.println("REGISTER USER: " + uid);
  drinkList = SD.open("newusers.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (drinkList) {
    Serial.print("Writing to newusers.txt...");
    drinkList.println(uid);
    // close the file:
    drinkList.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening newusers.txt");
  }
}

void sdCardSPI() {
  Serial.println("Listening to SD reader");
  digitalWrite(SD_SS_PIN, LOW);
  digitalWrite(RFID_SS_PIN, HIGH);
  delay(pulldowndelay);
}

void rfidSPI() {
  Serial.println("Listening to RFID reader");
  digitalWrite(SD_SS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, LOW);
  delay(pulldowndelay);
}

void noSPI() {
  Serial.println("Listening to no SPI reader");
  digitalWrite(SD_SS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, HIGH);
  delay(pulldowndelay);
}

void registerUserMode() {
  bool inRUM = true;
  blinkLEDs();
  digitalWrite(LED_BLUE,LOW);
  while(inRUM) {
    rfidSPI();
    digitalWrite(LED_RED,HIGH);
    digitalWrite(LED_GREEN,HIGH);
    waitForRFIDCard();

    String currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
    currentUid.trim();
  
    if (registerUsersId.equalsIgnoreCase(currentUid)){
      inRUM = false;
      continue;
    }
    
    sdCardSPI();

    if (doesUserNeedRegistering(currentUid)) {
      digitalWrite(LED_RED,LOW);
      registerNewUser(currentUid);
      delay(afterScanDelay);
    } else {
      digitalWrite(LED_GREEN,LOW);
      delay(afterScanDelay);  
    }
  }
  rfidSPI();
  blinkLEDs();
  delay(500);
  return;
}

void blinkLEDs() {
  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  delay(500);
  digitalWrite(LED_BLUE,LOW);
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_GREEN,LOW);
  delay(500);
  digitalWrite(LED_BLUE,HIGH);
  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  delay(500);
  digitalWrite(LED_BLUE,LOW);
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_GREEN,LOW);
  delay(500);
  digitalWrite(LED_BLUE,HIGH);
  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  delay(500);
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
