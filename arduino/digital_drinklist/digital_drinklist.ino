#include <MFRC522.h>
#include <SD.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#define SD_SS_PIN       4   // SD CARD
#define RST_PIN         9   // RFID
#define RFID_SS_PIN     8   // RFID
#define LED_RED         3
#define LED_GREEN       2
#define LED_BLUE        5
#define pulldowndelay   50
#define afterScanDelay  3000

MFRC522 mfrc522(RFID_SS_PIN, RST_PIN);  // Create MFRC522 (RFID) instance

/**
 * Identifier (UID) of administrative RFID tag.
 */
static String registerUsersId = "4920DBA3";

/**
 * File handle of `users.txt`.
 */
static File users = nullptr;

/**
 * File handle of `drinks.txt`.
 */
static File drinkList = nullptr;

/**
 * Program initialization.
 */
void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  // LED SETUP
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);

  // SPI SETUP
  SPI.begin();
  pinMode(SD_SS_PIN, OUTPUT);
  pinMode(RFID_SS_PIN, OUTPUT);
  noSPI();

  // RFID SETUP
  rfidSPI();
  Serial.print("Initializing RFID reader...");
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  //mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

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

/**
 * Program main loop.
 */
void loop() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  waitForRFIDCard();

  String currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
  currentUid.trim();

  if (registerUsersId.equalsIgnoreCase(currentUid)) {
    registerUserMode();
    return;
  }

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);

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

/**
 * Wait until card with readable UID is present on RFID sensor.
 */
void waitForRFIDCard() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial());
}

/**
 * Return the hexadecimal string representation of a buffer.
 *
 * Output representation is encoded in upper case ASCII.
 *
 * @param buffer     Buffer to convert.
 * @param bufferSize Size of buffer in bytes.
 * @return           Hexadecimal representation of buffer.
 *
 * Example:
 * ```cpp
 * byte *buffer = (byte *) "Foobar";
 * String repr = getUid(buffer, sizeof buffer);
 * assert(repr == "0F6F6F626172");
 * ```
 */
String getUid(byte *buffer, byte bufferSize) {
  String byteArray;
  for (byte i = 0; i < bufferSize; i++) {
    byteArray.concat(String(buffer[i], HEX));
    byteArray.toUpperCase();
  }
  return byteArray;
}

/**
 * Resolve a user's name from a UID.
 *
 * @param uid Identifier (UID) to resolve.
 * @return    Name of user belonging to identifier.
 */
String getNameFromUid(String uid) {
  users = SD.open("users.txt", FILE_READ);

  if (!users) {
    Serial.println("Error opening users.txt");
    return "NOUSER";
  }
  String row = "";
  String userId = "";
  String userName = "NOUSER";
  while (users.available() != 0) {
    row = users.readStringUntil('\n');
    if (row == "")
      break;

    userId = getValue(row, ':', 0);
    if (uid.equalsIgnoreCase(userId)) {
      userName = getValue(row, ':', 1);
      userName.trim();
      if (userName.equalsIgnoreCase(""))
        userName = "NOUSER";
    }
  }
  users.close();

  // Toggle LED's
  if (userName != "NOUSER")
    digitalWrite(LED_RED, LOW);
  else
    digitalWrite(LED_GREEN, LOW);

  return userName;
}

/**
 * Check if user is registered in database.
 *
 * @param uid Identifier (UID) of user to query.
 * @return    Presence of user in database.
 */
bool doesUserNeedRegistering(String uid) {
  users = SD.open("users.txt", FILE_READ);

  if (!users) {
    Serial.println("Error opening users.txt");
    return false;
  }
  String row = "";
  String userId = "";
  bool userInDB = true;
  while (users.available() != 0) {
    row = users.readStringUntil('\n');
    if (row == "")
      break;

    userId = getValue(row, ':', 0);
    if (uid.equalsIgnoreCase(userId)) {
      userInDB = false;
      break;
    }
  }
  users.close();

  return userInDB;
}

/**
 * Log a new ticket for a user.
 *
 * @param userName Identifier (UID) of user to log ticket for.
 */
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

/**
 * Register new user in persistent database.
 *
 * @param uid Identifier (UID) of new user.
 */
void registerNewUser(String uid) {
  Serial.println("REGISTER USER: " + uid);
  users = SD.open("users.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (users) {
    Serial.print("Writing to users.txt...");
    users.println(uid + ":");
    // close the file:
    users.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening users.txt");
  }
}

/**
 * Enable reading of SD card.
 */
void sdCardSPI() {
  Serial.println("Listening to SD reader");
  digitalWrite(SD_SS_PIN, LOW);
  digitalWrite(RFID_SS_PIN, HIGH);
  delay(pulldowndelay);
}

/**
 * Enable reading of RFID sensor.
 */
void rfidSPI() {
  Serial.println("Listening to RFID reader");
  digitalWrite(SD_SS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, LOW);
  delay(pulldowndelay);
}

/**
 * Enable reading of SD card and RFID sensor.
 */
void noSPI() {
  Serial.println("Listening to no SPI reader");
  digitalWrite(SD_SS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, HIGH);
  delay(pulldowndelay);
}

/**
 * Register RFID identities.
 *
 * Routine awaits activation of the RFID sensor. New UIDs presented to
 * the sensor are registered in the database. When the administration UID is
 * used, the routine exits. LED animations are performed during the routine.
 */
void registerUserMode() {
  blinkLEDs();
  digitalWrite(LED_BLUE, LOW);
  while (1) {
    rfidSPI();
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    waitForRFIDCard();

    String currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
    currentUid.trim();

    if (registerUsersId.equalsIgnoreCase(currentUid))
      break;

    sdCardSPI();

    if (doesUserNeedRegistering(currentUid)) {
      digitalWrite(LED_RED, LOW);
      registerNewUser(currentUid);
      delay(afterScanDelay);
    } else {
      digitalWrite(LED_GREEN, LOW);
      delay(afterScanDelay);
    }
  }
  rfidSPI();
  blinkLEDs();
  delay(500);
}

/**
 * Cycle all LEDs twice a second, for a duration of two and a half seconds.
 *
 * Red and green LEDs are activated, followed by a double cycle of LEDs cycling
 * off and on. At the end of the routine, all LEDs are active.
 */
void blinkLEDs() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  delay(500);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  delay(500);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
}

/**
 * Extract the N:th segment of a delimited sting.
 *
 * @param data      String to search.
 * @param separator Segment separator. Usually not present in output.
 * @param index     Segment to extract, starting from `0`.
 * @return          A copy of the substring, of `""` on no match.
 *
 * Example:
 * ```cpp
 * String segment = getValue("foo,bar,baz,quux", ',', 2);
 * assert(segment == "baz");
 * ```
 */
String getValue(String data, char separator, size_t index) {
  uintmax_t found = 0;
  ssize_t strIndex[] = { 0, -1 };
  size_t maxIndex = data.length() - 1;

  for (size_t i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return (found > index) ? data.substring(strIndex[0], strIndex[1]) : "";
}
