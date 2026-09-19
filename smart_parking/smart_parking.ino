#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID = "KaushikBedroom";
const char* WIFI_PASSWORD = "apSJW@1100";

// ============================================================
// FLASK SERVER
// ============================================================

const char* FLASK_SERVER = "192.168.31.140";
const int FLASK_PORT = 5000;

// ============================================================
// PIN CONFIGURATION
// ============================================================

// Parking slots
#define SLOT2_IR 35
#define SLOT3_IR 32
#define SLOT4_IR 33

// Entry sensor
#define ENTRY_IR 25

// RC522
#define RFID_SS 21
#define RFID_RST 22

// SPI
#define RFID_SCK 18
#define RFID_MISO 19
#define RFID_MOSI 23

// Servo
#define SERVO_PIN 27

// LCD
// SDA = GPIO 4
// SCL = RX2 = GPIO 16
#define LCD_SDA 4
#define LCD_SCL 16
#define LCD_ADDRESS 0x27

// ============================================================
// OBJECTS
// ============================================================

MFRC522 rfid(
  RFID_SS,
  RFID_RST
);

LiquidCrystal_I2C lcd(
  LCD_ADDRESS,
  16,
  2
);

Servo barrier;

// ============================================================
// SETTINGS
// ============================================================

// LOW = object detected
const bool IR_ACTIVE_LOW = true;

// Servo angles
const int BARRIER_CLOSED = 0;
const int BARRIER_OPEN = 90;

// Maximum time allowed for vehicle passage
const unsigned long PASSAGE_TIMEOUT = 15000;

// Flask update interval
const unsigned long FLASK_UPDATE_INTERVAL = 1000;

// Sensor debug interval
const unsigned long SENSOR_DEBUG_INTERVAL = 2000;

// WiFi retry interval
const unsigned long WIFI_RETRY_INTERVAL = 10000;

// ============================================================
// AUTHORIZED RFID
// ============================================================

struct AuthorizedCard {

  byte uid[10];

  byte uidSize;

  const char* name;
};

AuthorizedCard authorizedCards[] = {

  {
    {0xAD, 0x4B, 0x29, 0x07},
    4,
    "RFID Tag"
  },

  {
    {0x2E, 0x60, 0xCA, 0x2A},
    4,
    "Card 1"
  },

  {
    {0x61, 0x4E, 0xD2, 0x06},
    4,
    "Card 2"
  },

  {
    {0x1E, 0x4D, 0xD6, 0x2A},
    4,
    "Card 3"
  }
};

const int AUTHORIZED_COUNT =
  sizeof(authorizedCards) /
  sizeof(authorizedCards[0]);

// ============================================================
// PARKING VARIABLES
// ============================================================

bool slot2Occupied = false;
bool slot3Occupied = false;
bool slot4Occupied = false;

bool vehicleDetected = false;

bool barrierIsOpen = false;

String entryStatus = "WAITING";
String barrierStatus = "CLOSED";

String lastRFID = "NONE";
String lastUser = "NONE";

unsigned long barrierOpenedAt = 0;
unsigned long lastFlaskUpdate = 0;
unsigned long lastSensorDebug = 0;
unsigned long lastWiFiRetry = 0;

// ============================================================
// IR SENSOR
// ============================================================

bool isObjectDetected(int pin) {

  int state = digitalRead(pin);

  if (IR_ACTIVE_LOW) {
    return state == LOW;
  }

  return state == HIGH;
}

// ============================================================
// READ PARKING SLOTS
// ============================================================

void readParkingSlots() {

  slot2Occupied = isObjectDetected(SLOT2_IR);

  slot3Occupied = isObjectDetected(SLOT3_IR);

  slot4Occupied = isObjectDetected(SLOT4_IR);

  vehicleDetected = isObjectDetected(ENTRY_IR);
}

// ============================================================
// SENSOR DEBUG
// ============================================================

void printSensorStatus() {

  Serial.println();
  Serial.println("----------------------------------------");

  Serial.print("IR RAW: ");

  Serial.print(digitalRead(SLOT2_IR));

  Serial.print(" | ");

  Serial.print(digitalRead(SLOT3_IR));

  Serial.print(" | ");

  Serial.print(digitalRead(SLOT4_IR));

  Serial.print(" | Entry: ");

  Serial.println(digitalRead(ENTRY_IR));

  Serial.print("SLOTS: ");

  Serial.print(
    slot2Occupied
      ? "P2 OCCUPIED"
      : "P2 FREE"
  );

  Serial.print(" | ");

  Serial.print(
    slot3Occupied
      ? "P3 OCCUPIED"
      : "P3 FREE"
  );

  Serial.print(" | ");

  Serial.println(
    slot4Occupied
      ? "P4 OCCUPIED"
      : "P4 FREE"
  );

  Serial.print("ENTRY: ");

  Serial.println(
    vehicleDetected
      ? "VEHICLE DETECTED"
      : "CLEAR"
  );

  Serial.println("----------------------------------------");
}

// ============================================================
// AVAILABLE SLOTS
// ============================================================

int getAvailableSlots() {

  int available = 0;

  if (!slot2Occupied) {
    available++;
  }

  if (!slot3Occupied) {
    available++;
  }

  if (!slot4Occupied) {
    available++;
  }

  return available;
}

// ============================================================
// LCD
// ============================================================

void showLCD(String line1, String line2 = "") {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1);

  if (line2.length() > 0) {

    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

// ============================================================
// LCD STATES
// ============================================================

void lcdScanRFID() {

  showLCD(
    "SCAN RFID",
    ""
  );
}

void lcdAccessGranted() {

  showLCD(
    "ACCESS GRANTED",
    ""
  );
}

void lcdGateOpen() {

  showLCD(
    "GATE OPEN",
    ""
  );
}

void lcdDriveThrough() {

  showLCD(
    "DRIVE THROUGH",
    ""
  );
}

void lcdGateClosing() {

  showLCD(
    "GATE CLOSING",
    ""
  );
}

void lcdAccessDenied() {

  showLCD(
    "ACCESS DENIED",
    ""
  );
}

void lcdParkingFull() {

  showLCD(
    "PARKING FULL",
    ""
  );
}

// ============================================================
// RFID UID
// ============================================================

String getUIDString() {

  String uidString = "";

  for (
    byte i = 0;
    i < rfid.uid.size;
    i++
  ) {

    if (rfid.uid.uidByte[i] < 0x10) {

      uidString += "0";
    }

    uidString += String(
      rfid.uid.uidByte[i],
      HEX
    );

    if (i < rfid.uid.size - 1) {

      uidString += ":";
    }
  }

  uidString.toUpperCase();

  return uidString;
}

// ============================================================
// AUTHORIZED USER
// ============================================================

String getAuthorizedUser() {

  for (
    int card = 0;
    card < AUTHORIZED_COUNT;
    card++
  ) {

    if (
      rfid.uid.size !=
      authorizedCards[card].uidSize
    ) {

      continue;
    }

    bool match = true;

    for (
      byte i = 0;
      i < rfid.uid.size;
      i++
    ) {

      if (
        rfid.uid.uidByte[i] !=
        authorizedCards[card].uid[i]
      ) {

        match = false;

        break;
      }
    }

    if (match) {

      return String(
        authorizedCards[card].name
      );
    }
  }

  return "";
}

// ============================================================
// OPEN BARRIER
// ============================================================

void openBarrier() {

  barrier.write(
    BARRIER_OPEN
  );

  barrierIsOpen = true;

  barrierStatus = "OPEN";

  barrierOpenedAt = millis();

  Serial.println("BARRIER OPENED");

  // LCD
  lcdGateOpen();
}

// ============================================================
// CLOSE BARRIER
// ============================================================

void closeBarrier() {

  Serial.println("GATE CLOSING");

  // LCD first
  lcdGateClosing();

  delay(1000);

  // Close servo
  barrier.write(
    BARRIER_CLOSED
  );

  barrierIsOpen = false;

  barrierStatus = "CLOSED";

  vehicleDetected = false;

  entryStatus = "WAITING";

  Serial.println("BARRIER CLOSED");

  delay(500);

  // Return to normal state
  lcdScanRFID();
}

// ============================================================
// RFID CHECK
// ============================================================

void checkRFID() {

  // Do not scan while gate is open
  if (barrierIsOpen) {

    return;
  }

  // No new card
  if (!rfid.PICC_IsNewCardPresent()) {

    return;
  }

  // Cannot read card
  if (!rfid.PICC_ReadCardSerial()) {

    return;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("RFID CARD/TAG DETECTED");

  // Get UID
  lastRFID = getUIDString();

  Serial.print("UID: ");
  Serial.println(lastRFID);

  // Check authorization
  String user = getAuthorizedUser();

  // ==========================================================
  // AUTHORIZED RFID
  // ==========================================================

  if (user != "") {

    lastUser = user;

    Serial.print("AUTHORIZED: ");
    Serial.println(user);

    int available =
      getAvailableSlots();

    Serial.print("Available slots: ");
    Serial.println(available);

    // ========================================================
    // PARKING FULL
    // ========================================================

    if (available <= 0) {

      entryStatus =
        "PARKING FULL";

      Serial.println(
        "ACCESS DENIED - PARKING FULL"
      );

      lcdParkingFull();

      delay(2500);

      lcdScanRFID();
    }

    // ========================================================
    // PARKING AVAILABLE
    // ========================================================

    else {

      entryStatus =
        "ACCESS GRANTED";

      Serial.println(
        "ACCESS GRANTED"
      );

      // LCD
      lcdAccessGranted();

      delay(1500);

      // Open gate
      openBarrier();
    }
  }

  // ==========================================================
  // UNAUTHORIZED RFID
  // ==========================================================

  else {

    lastUser =
      "UNAUTHORIZED";

    entryStatus =
      "ACCESS DENIED";

    Serial.println(
      "UNAUTHORIZED RFID"
    );

    // LCD
    lcdAccessDenied();

    delay(2500);

    lcdScanRFID();
  }

  // Stop RFID communication
  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();

  Serial.println("========================================");
}

// ============================================================
// CONNECT WIFI
// ============================================================

bool connectWiFi() {

  Serial.println();
  Serial.println("========================================");
  Serial.println("CONNECTING TO WIFI");
  Serial.println("========================================");

  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

  WiFi.disconnect(true);

  delay(500);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  ) {

    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WIFI CONNECTED");

    Serial.println("----------------------------------------");

    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());

    Serial.print("Signal RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.println("----------------------------------------");

    Serial.print("Flask Server: http://");
    Serial.print(FLASK_SERVER);
    Serial.print(":");
    Serial.print(FLASK_PORT);
    Serial.println("/api/update");

    Serial.println("========================================");

    return true;
  }

  Serial.println();
  Serial.println("WIFI CONNECTION FAILED");

  Serial.println("========================================");

  return false;
}

// ============================================================
// WIFI STATUS
// ============================================================

void checkWiFiConnection() {

  if (WiFi.status() == WL_CONNECTED) {

    return;
  }

  if (
    millis() - lastWiFiRetry >=
    WIFI_RETRY_INTERVAL
  ) {

    lastWiFiRetry = millis();

    Serial.println();
    Serial.println(
      "WiFi disconnected."
    );

    Serial.println(
      "Attempting WiFi reconnect..."
    );

    WiFi.disconnect();

    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );
  }
}

// ============================================================
// SEND DATA TO FLASK
// ============================================================

void sendToFlask() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "Flask update skipped - WiFi disconnected"
    );

    return;
  }

  String url =
    "http://" +
    String(FLASK_SERVER) +
    ":" +
    String(FLASK_PORT) +
    "/api/update";

  Serial.println();
  Serial.println("========================================");
  Serial.println("FLASK UPDATE");
  Serial.println("----------------------------------------");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Server: ");
  Serial.println(url);

  // ==========================================================
  // JSON
  // ==========================================================

  String json = "{";

  json +=
    "\"slot2\":" +
    String(
      slot2Occupied
        ? "true"
        : "false"
    ) +
    ",";

  json +=
    "\"slot3\":" +
    String(
      slot3Occupied
        ? "true"
        : "false"
    ) +
    ",";

  json +=
    "\"slot4\":" +
    String(
      slot4Occupied
        ? "true"
        : "false"
    ) +
    ",";

  json +=
    "\"entry_status\":\"" +
    entryStatus +
    "\",";

  json +=
    "\"barrier\":\"" +
    barrierStatus +
    "\",";

  json +=
    "\"vehicle_detected\":" +
    String(
      vehicleDetected
        ? "true"
        : "false"
    ) +
    ",";

  json +=
    "\"last_rfid\":\"" +
    lastRFID +
    "\",";

  json +=
    "\"last_user\":\"" +
    lastUser +
    "\"";

  json += "}";

  Serial.println("JSON:");
  Serial.println(json);

  // ==========================================================
  // HTTP
  // ==========================================================

  HTTPClient http;

  http.setConnectTimeout(5000);

  http.setTimeout(5000);

  if (!http.begin(url)) {

    Serial.println();
    Serial.println(
      "ERROR: http.begin() FAILED"
    );

    Serial.println("========================================");

    return;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  Serial.println(
    "Sending POST..."
  );

  int httpCode =
    http.POST(json);

  Serial.print(
    "HTTP Response: "
  );

  Serial.println(httpCode);

  // ==========================================================
  // RESPONSE
  // ==========================================================

  if (httpCode > 0) {

    String response =
      http.getString();

    Serial.print(
      "Server Response: "
    );

    Serial.println(response);

    if (httpCode == 200) {

      Serial.println(
        "SUCCESS: Flask received the data."
      );
    }

    else {

      Serial.print(
        "Flask returned HTTP code: "
      );

      Serial.println(httpCode);
    }
  }

  else {

    Serial.println();
    Serial.println(
      "ERROR: Could not connect to Flask."
    );

    Serial.print(
      "HTTP Error: "
    );

    Serial.println(
      http.errorToString(httpCode)
    );

    Serial.println();
    Serial.println(
      "NETWORK INFORMATION"
    );

    Serial.println("----------------------------------------");

    Serial.print(
      "ESP32 IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

    Serial.print(
      "Gateway: "
    );

    Serial.println(
      WiFi.gatewayIP()
    );

    Serial.print(
      "Subnet: "
    );

    Serial.println(
      WiFi.subnetMask()
    );

    Serial.print(
      "Flask IP: "
    );

    Serial.println(
      FLASK_SERVER
    );

    Serial.print(
      "Flask Port: "
    );

    Serial.println(
      FLASK_PORT
    );

    Serial.println(
      "----------------------------------------"
    );
  }

  http.end();

  Serial.println(
    "========================================"
  );
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "       SMART PARKING SYSTEM"
  );

  Serial.println(
    "========================================"
  );

  // ==========================================================
  // IR SENSORS
  // ==========================================================

  pinMode(
    SLOT2_IR,
    INPUT
  );

  pinMode(
    SLOT3_IR,
    INPUT
  );

  pinMode(
    SLOT4_IR,
    INPUT
  );

  pinMode(
    ENTRY_IR,
    INPUT
  );

  // ==========================================================
  // LCD
  // ==========================================================

  Wire.begin(
    LCD_SDA,
    LCD_SCL
  );

  lcd.init();

  lcd.backlight();

  showLCD(
    "SMART PARKING",
    "Starting..."
  );

  delay(1000);

  // ==========================================================
  // SERVO
  // ==========================================================

  barrier.attach(
    SERVO_PIN
  );

  barrier.write(
    BARRIER_CLOSED
  );

  barrierIsOpen = false;

  barrierStatus = "CLOSED";

  // ==========================================================
  // RFID
  // ==========================================================

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  rfid.PCD_Init();

  delay(100);

  // ==========================================================
  // WIFI
  // ==========================================================

  connectWiFi();

  // ==========================================================
  // SENSOR READING
  // ==========================================================

  readParkingSlots();

  printSensorStatus();

  // ==========================================================
  // LCD READY
  // ==========================================================

  lcdScanRFID();

  // ==========================================================
  // INITIAL FLASK UPDATE
  // ==========================================================

  if (
    WiFi.status() == WL_CONNECTED
  ) {

    sendToFlask();
  }

  Serial.println();
  Serial.println(
    "SYSTEM READY"
  );

  Serial.println(
    "========================================"
  );
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // ==========================================================
  // WIFI
  // ==========================================================

  checkWiFiConnection();

  // ==========================================================
  // READ SENSORS
  // ==========================================================

  readParkingSlots();

  // ==========================================================
  // RFID
  // ==========================================================

  checkRFID();

  // ==========================================================
  // BARRIER
  // ==========================================================

  if (barrierIsOpen) {

    // --------------------------------------------------------
    // VEHICLE HAS REACHED ENTRY SENSOR
    // --------------------------------------------------------

    if (vehicleDetected) {

      entryStatus =
        "VEHICLE PASSING";

      Serial.println(
        "VEHICLE DETECTED AT ENTRY"
      );

      // LCD
      lcdDriveThrough();

      unsigned long start =
        millis();

      // ------------------------------------------------------
      // WAIT UNTIL VEHICLE CLEARS ENTRY SENSOR
      // ------------------------------------------------------

      while (
        isObjectDetected(ENTRY_IR) &&
        millis() - start < 10000
      ) {

        delay(100);

        readParkingSlots();
      }

      // ------------------------------------------------------
      // VEHICLE PASSED
      // ------------------------------------------------------

      closeBarrier();
    }

    // --------------------------------------------------------
    // VEHICLE DID NOT PASS
    // --------------------------------------------------------

    else if (
      millis() -
      barrierOpenedAt >
      PASSAGE_TIMEOUT
    ) {

      Serial.println(
        "PASSAGE TIMEOUT"
      );

      closeBarrier();
    }
  }

  // ==========================================================
  // SEND DATA TO FLASK EVERY SECOND
  // ==========================================================

  if (
    millis() -
    lastFlaskUpdate >=
    FLASK_UPDATE_INTERVAL
  ) {

    lastFlaskUpdate =
      millis();

    sendToFlask();
  }

  // ==========================================================
  // SENSOR DEBUG
  // ==========================================================

  if (
    millis() -
    lastSensorDebug >=
    SENSOR_DEBUG_INTERVAL
  ) {

    lastSensorDebug =
      millis();

    printSensorStatus();
  }

  // ==========================================================
  // SMALL LOOP DELAY
  // ==========================================================

  delay(50);
}