# Smart Parking System

ESP32 + RC522 RFID + 4 Parking IR Sensors + Entry IR + Servo Barrier + I2C LCD + Flask Dashboard.

## 1. Flask server

Open a terminal in this folder:

    python -m venv venv

Windows:

    venv\Scripts\activate

Linux/macOS:

    source venv/bin/activate

Install:

    pip install -r requirements.txt

Run:

    python app.py

The dashboard will be available at:

    http://127.0.0.1:5000

For another computer/phone on the same Wi-Fi:

    http://YOUR_PC_IP:5000

Find the PC IP with `ipconfig` on Windows or `ip addr` on Linux.

## 2. ESP32

Install these Arduino libraries:

- MFRC522
- LiquidCrystal_I2C
- ESP32Servo

Edit these values in `smart_parking.ino`:

    WIFI_SSID
    WIFI_PASSWORD
    FLASK_SERVER

Example:

    const char* FLASK_SERVER = "192.168.1.100";

This must be the LAN IP of the PC running Flask.

## 3. RFID authorization

Replace the example UID:

    { {0x12, 0x34, 0x56, 0x78}, 4, "Authorized User" }

with your actual RFID UID.

Open Serial Monitor at 115200 baud and scan the card. The UID will be printed.

## 4. IR sensors

The code assumes the IR modules are ACTIVE LOW:

    LOW  = vehicle/object detected
    HIGH = clear

If your modules behave opposite, change:

    const bool IR_ACTIVE_LOW = true;

to:

    const bool IR_ACTIVE_LOW = false;

## 5. Important electrical notes

RC522 must be powered from 3.3 V.

ESP32 GPIOs are not 5 V tolerant. Do not connect a 5 V IR output directly to an ESP32 GPIO.

The servo should use an external 5 V supply. Connect the servo supply GND to ESP32 GND.

If your I2C LCD backpack uses 5 V pull-ups, use a suitable level shifter or otherwise ensure SDA/SCL never exceed 3.3 V at the ESP32.

## 6. System flow

RFID scanned
    -> UID checked
    -> if authorized and a slot is available
    -> servo opens
    -> entry IR detects vehicle
    -> vehicle clears entry sensor
    -> servo closes
    -> slot status continues to be reported
    -> Flask dashboard updates

The Flask dashboard polls `/api/status` every second.

The ESP32 sends data to:

    POST /api/update
