# Smart Parking System

ESP32 + RC522 RFID + 3 Parking IR Sensors + Entry IR + Servo Barrier + I2C LCD + Flask Dashboard.

## System Block Diagram

<p align="center">
  <img src="block-diagram.png" alt="Smart Parking System Block Diagram" width="800">
</p>

## 1. Flask Server

Open a terminal in this folder:

    python -m venv venv

### Windows

    venv\Scripts\activate

### Linux/macOS

    source venv/bin/activate

Install the required Python packages:

    pip install -r requirements.txt

Run the Flask server:

    python app.py

The dashboard will be available at:

    http://127.0.0.1:5000

For another computer or phone on the same Wi-Fi:

    http://YOUR_PC_IP:5000

Find the PC IP with:

    ipconfig

on Windows, or:

    ip addr

on Linux.

## 2. ESP32

Install these Arduino libraries:

- MFRC522
- LiquidCrystal_I2C
- ESP32Servo

Configure these values in the ESP32 code:

    WIFI_SSID
    WIFI_PASSWORD
    FLASK_SERVER

Example:

    const char* FLASK_SERVER = "192.168.1.100";

This must be the LAN IP address of the computer running Flask.

## 3. Hardware Configuration

### Parking Slots

The system uses three parking slots:

- Slot 2
- Slot 3
- Slot 4

Each slot uses an IR sensor to detect whether the slot is occupied.

### Pin Configuration

| Component | ESP32 GPIO |
|---|---:|
| Slot 2 IR | GPIO 35 |
| Slot 3 IR | GPIO 32 |
| Slot 4 IR | GPIO 33 |
| Entry IR | GPIO 25 |
| RC522 SDA/SS | GPIO 21 |
| RC522 RST | GPIO 22 |
| RC522 SCK | GPIO 18 |
| RC522 MISO | GPIO 19 |
| RC522 MOSI | GPIO 23 |
| Servo | GPIO 27 |
| LCD SDA | GPIO 4 |
| LCD SCL | GPIO 16 / RX2 |

The I2C LCD uses address:

    0x27

## 4. RFID Authorization

The system checks the UID of every scanned RFID card against the authorized RFID list.

To add or replace an RFID card, update the authorized card list in the ESP32 code.

Open Serial Monitor at 115200 baud and scan the card. The UID will be printed.

## 5. LCD Display States

The LCD provides feedback during the parking process.

| Situation | LCD Display |
|---|---|
| Normal / waiting | `SCAN RFID` |
| RFID recognized + slot available | `ACCESS GRANTED` |
| Barrier opening | `GATE OPEN` |
| Vehicle passing | `DRIVE THROUGH` |
| Vehicle passed | `GATE CLOSING` |
| RFID not authorized | `ACCESS DENIED` |
| No parking available | `PARKING FULL` |

## 6. IR Sensors

The code assumes the IR modules are ACTIVE LOW:

    LOW  = vehicle/object detected
    HIGH = clear

If your modules behave in the opposite way, change:

    const bool IR_ACTIVE_LOW = true;

to:

    const bool IR_ACTIVE_LOW = false;

## 7. Important Electrical Notes

RC522 must be powered from 3.3 V.

ESP32 GPIOs are not 5 V tolerant. Do not connect a 5 V IR output directly to an ESP32 GPIO.

The servo should use an external 5 V supply.

Connect the servo supply GND to ESP32 GND to maintain a common ground.

If the I2C LCD backpack uses 5 V pull-ups, use a suitable level shifter or otherwise ensure SDA/SCL never exceed 3.3 V at the ESP32.

## 8. System Flow

    RFID scanned
          ↓
    UID checked
          ↓
    ┌─────────────────────────┐
    │ Authorized RFID?        │
    └─────────────────────────┘
          ↓ Yes
    Check available slots
          ↓
    ┌─────────────────────────┐
    │ Parking available?      │
    └─────────────────────────┘
       ↓ Yes          ↓ No
    Access Granted   Parking Full
       ↓
    Gate Opens
       ↓
    Entry IR detects vehicle
       ↓
    DRIVE THROUGH
       ↓
    Vehicle clears Entry IR
       ↓
    Gate Closing
       ↓
    Gate Closed
       ↓
    SCAN RFID

## 9. Flask Dashboard

The Flask dashboard displays the current parking status and barrier information.

The ESP32 sends parking and gate information to:

    POST /api/update

The dashboard retrieves the latest system status through:

    GET /api/status

The dashboard polls the status endpoint every second.

## 10. Data Sent by ESP32

The ESP32 sends information including:

- Slot 2 occupancy
- Slot 3 occupancy
- Slot 4 occupancy
- Entry status
- Barrier status
- Vehicle detection
- Last RFID UID
- Last authorized user

## 11. Project Structure

    smart_parking_system/
    │
    ├── app.py
    ├── index.html
    ├── requirements.txt
    ├── README.md
    ├── block-diagram.png
    │
    ├── smart_parking/
    │   └── smart_parking.ino
    │
    └── templates/
        └── ...

## 12. Running the Project

### Start Flask

    python app.py

### Upload ESP32 Code

1. Open the `.ino` file in Arduino IDE.
2. Select the ESP32 board.
3. Select the correct COM port.
4. Upload the code.
5. Open Serial Monitor at 115200 baud.

### Open Dashboard

On the PC:

    http://127.0.0.1:5000

From another device on the same network:

    http://YOUR_PC_IP:5000
