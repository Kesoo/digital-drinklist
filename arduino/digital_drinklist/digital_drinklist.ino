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
static const String registerUsersId = "4920DBA3";

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
  // Setup serial interface.
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  // Setup LED actuators.
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);

  // Setup SPI interface.
  SPI.begin();
  pinMode(SD_SS_PIN, OUTPUT);
  pinMode(RFID_SS_PIN, OUTPUT);
  noSPI();

  // Setup RFID reader.
  rfidSPI();
  Serial.print("Initializing RFID reader...");
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  //mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  // Setup SD card.
  sdCardSPI();
  Serial.print("Initializing SD card...");

  // Verify correct initialization.
  if (!SD.begin(SD_SS_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // Enable RFID sensor.
  rfidSPI();
}

/**
 * Program main loop.
 */
void loop() {
  // ...
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // Await positive readback from RFID reader.
  waitForRFIDCard();

  // Read UID from RFID sensor.
  auto currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
  currentUid.trim();

  // Recognize administrative UID.
  if (registerUsersId.equalsIgnoreCase(currentUid)) {
    registerUserMode();
    return;
  }

  // ...
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  // Resolve UID.
  sdCardSPI();
  Serial.println("Current Uid: " + currentUid);
  const auto userName = getNameFromUid(currentUid);

  // Register ticket.
  if (userName != "NOUSER") {
    Serial.println("USER FOUND");
    registerDrink(userName);
  } else {
    Serial.println("NO USER FOUND");
  }

  // Delay next scan.
  rfidSPI();
  delay(afterScanDelay);
}

/**
 * Wait until card with readable UID is present on RFID sensor.
 */
void waitForRFIDCard() {
  // Block until a card or a serial number can be detected over RFID.
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
String getUid(const byte *buffer, const byte bufferSize) {
  // Construct output string, byte-by-byte.
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
String getNameFromUid(const String uid) {
  // Current resolved username.
  String userName = "NOUSER";

  // Open database for reading.
  users = SD.open("users.txt", FILE_READ);
  if (!users) {
    Serial.println("Error opening users.txt");
    return userName;
  }

  // Search database.
  while (users.available() != 0) {
    // Read a single entry.
    const auto row = users.readStringUntil('\n');

    // Skip empty lines.
    if (row == "")
      break;

    // Read the UID field of entry.
    const auto userId = getValue(row, ':', 0);

    // Filter by UID.
    if (uid.equalsIgnoreCase(userId)) {
      // Resolve username of entry.
      userName = getValue(row, ':', 1);
      userName.trim();
      if (userName.equalsIgnoreCase(""))
        userName = "NOUSER";
    }
  }

  // Close database.
  users.close();

  // Update LEDs.
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
bool doesUserNeedRegistering(const String uid) {
  // Open database for reading.
  users = SD.open("users.txt", FILE_READ);
  if (!users) {
    Serial.println("Error opening users.txt");
    return false;
  }

  // Find UID in database.
  bool userInDB = true;
  while (users.available() != 0) {
    // Read a single entry.
    const auto row = users.readStringUntil('\n');

    // Skip empty lines.
    if (row == "")
      break;

    // Read the UID field of entry.
    const auto userId = getValue(row, ':', 0);

    // Compare against needle.
    if (uid.equalsIgnoreCase(userId)) {
      userInDB = false;
      break;
    }
  }

  // Close database.
  users.close();

  return userInDB;
}

/**
 * Log a new ticket for a user.
 *
 * @param userName Identifier (UID) of user to log ticket for.
 */
void registerDrink(const String userName) {
  Serial.println("REGISTER DRINK: " + userName);

  // Open log.
  drinkList = SD.open("drinks.txt", FILE_WRITE);
  if (!drinkList) {
    Serial.println("error opening drinks.txt");
    return;
  }

  // Append to log.
  Serial.print("Writing to drinks.txt...");
  userName.trim();
  drinkList.println(userName);

  // Close the log.
  drinkList.close();

  Serial.println("done.");
}

/**
 * Register new user in persistent database.
 *
 * @param uid Identifier (UID) of new user.
 */
void registerNewUser(const String uid) {
  Serial.println("REGISTER USER: " + uid);

  // Open database for writing.
  users = SD.open("users.txt", FILE_WRITE);
  if (!users) {
    Serial.println("error opening users.txt");
    return;
  }

  // Append to database.
  Serial.print("Writing to users.txt...");
  users.println(uid + ":");

  // Close database.
  users.close();

  Serial.println("done.");
}

/**
 * Enable reading of SD card.
 */
void sdCardSPI() {
  // Configure pins for SD card reader.
  Serial.println("Listening to SD reader");
  digitalWrite(SD_SS_PIN, LOW);
  digitalWrite(RFID_SS_PIN, HIGH);
  delay(pulldowndelay);
}

/**
 * Enable reading of RFID sensor.
 */
void rfidSPI() {
  // Configure pins for RFID reader.
  Serial.println("Listening to RFID reader");
  digitalWrite(SD_SS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, LOW);
  delay(pulldowndelay);
}

/**
 * Enable reading of SD card and RFID sensor.
 */
void noSPI() {
  // Configure pins neither reader.
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
  // Perform LED sequence.
  blinkLEDs();
  digitalWrite(LED_BLUE, LOW);

  // Process UIDs.
  while (1) {
    rfidSPI();
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    waitForRFIDCard();

    auto currentUid = getUid(mfrc522.uid.uidByte, mfrc522.uid.size);
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

  // Restore state.
  rfidSPI();

  // Indicate before returning to previous mode.
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
String getValue(const String data, const char separator, const size_t index) {
  uintmax_t found = 0;
  ssize_t strIndex[] = { 0, -1 };
  const size_t maxIndex = data.length() - 1;
  // TODO: assert(maxIndex <= SIZE_MAX);

  // Scan input buffer.
  for (size_t i = 0; i <= maxIndex && found <= index; i++) {
    // Search for delimiter.
    if (data.charAt(i) == separator || i == maxIndex) {
      // Store data of current segment.
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  // Return segment if found.
  return (found > index) ? data.substring(strIndex[0], strIndex[1]) : "";
}
