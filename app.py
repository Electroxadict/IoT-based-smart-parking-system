from flask import Flask, render_template, request, jsonify
from datetime import datetime
import time

app = Flask(__name__)

# ============================================================
# CONFIGURATION
# ============================================================

PC_IP = "192.168.31.140"
PORT = 5000

ESP32_TIMEOUT = 10


# ============================================================
# PARKING DATA
# ONLY SLOT 2, SLOT 3 AND SLOT 4
# ============================================================

parking_data = {

    "slot2": False,
    "slot3": False,
    "slot4": False,

    "entry_status": "WAITING",
    "barrier": "CLOSED",
    "vehicle_detected": False,

    "last_rfid": "NONE",
    "last_user": "NONE",

    "last_update": "Never",
    "last_update_timestamp": 0
}


# ============================================================
# CHECK ESP32 CONNECTION
# ============================================================

def is_esp32_online():

    last_update = parking_data["last_update_timestamp"]

    if last_update == 0:
        return False

    return (
        time.time() - last_update
    ) <= ESP32_TIMEOUT


# ============================================================
# DASHBOARD
# ============================================================

@app.route("/")
def dashboard():

    return render_template("index.html")


# ============================================================
# GET PARKING STATUS
# ============================================================

@app.route("/api/status", methods=["GET"])
def get_status():

    available = sum([
        not parking_data["slot2"],
        not parking_data["slot3"],
        not parking_data["slot4"]
    ])

    occupied = 3 - available

    return jsonify({

        # Parking slots
        "slot2": parking_data["slot2"],
        "slot3": parking_data["slot3"],
        "slot4": parking_data["slot4"],

        # Parking summary
        "total_slots": 3,
        "available_slots": available,
        "occupied_slots": occupied,

        # Entry system
        "entry_status":
            parking_data["entry_status"],

        "barrier":
            parking_data["barrier"],

        "vehicle_detected":
            parking_data["vehicle_detected"],

        # RFID
        "last_rfid":
            parking_data["last_rfid"],

        "last_user":
            parking_data["last_user"],

        # Connection
        "esp32_online":
            is_esp32_online(),

        "last_update":
            parking_data["last_update"],

        "server_time":
            datetime.now().strftime(
                "%d-%m-%Y %H:%M:%S"
            )
    })


# ============================================================
# ESP32 → FLASK
# ============================================================

@app.route("/api/update", methods=["POST"])
def update_status():

    data = request.get_json(silent=True)

    if data is None:

        print("ERROR: Invalid JSON received")

        return jsonify({
            "success": False,
            "message": "Invalid JSON"
        }), 400


    # ========================================================
    # SLOT 2
    # ========================================================

    if "slot2" in data:

        value = data["slot2"]

        if isinstance(value, bool):

            parking_data["slot2"] = value

        elif isinstance(value, str):

            parking_data["slot2"] = (
                value.lower()
                in ["true", "1", "occupied"]
            )

        else:

            parking_data["slot2"] = bool(value)


    # ========================================================
    # SLOT 3
    # ========================================================

    if "slot3" in data:

        value = data["slot3"]

        if isinstance(value, bool):

            parking_data["slot3"] = value

        elif isinstance(value, str):

            parking_data["slot3"] = (
                value.lower()
                in ["true", "1", "occupied"]
            )

        else:

            parking_data["slot3"] = bool(value)


    # ========================================================
    # SLOT 4
    # ========================================================

    if "slot4" in data:

        value = data["slot4"]

        if isinstance(value, bool):

            parking_data["slot4"] = value

        elif isinstance(value, str):

            parking_data["slot4"] = (
                value.lower()
                in ["true", "1", "occupied"]
            )

        else:

            parking_data["slot4"] = bool(value)


    # ========================================================
    # ENTRY STATUS
    # ========================================================

    if "entry_status" in data:

        parking_data["entry_status"] = str(
            data["entry_status"]
        )


    # ========================================================
    # BARRIER
    # ========================================================

    if "barrier" in data:

        parking_data["barrier"] = str(
            data["barrier"]
        )


    # ========================================================
    # VEHICLE DETECTED
    # ========================================================

    if "vehicle_detected" in data:

        value = data["vehicle_detected"]

        if isinstance(value, bool):

            parking_data["vehicle_detected"] = value

        elif isinstance(value, str):

            parking_data["vehicle_detected"] = (
                value.lower()
                in ["true", "1", "yes"]
            )

        else:

            parking_data["vehicle_detected"] = bool(value)


    # ========================================================
    # RFID
    # ========================================================

    if "last_rfid" in data:

        parking_data["last_rfid"] = str(
            data["last_rfid"]
        )


    # ========================================================
    # USER
    # ========================================================

    if "last_user" in data:

        parking_data["last_user"] = str(
            data["last_user"]
        )


    # ========================================================
    # TIMESTAMP
    # ========================================================

    now = datetime.now()

    parking_data["last_update"] = (
        now.strftime("%d-%m-%Y %H:%M:%S")
    )

    parking_data["last_update_timestamp"] = time.time()


    # ========================================================
    # FLASK TERMINAL OUTPUT
    # ========================================================

    available = sum([
        not parking_data["slot2"],
        not parking_data["slot3"],
        not parking_data["slot4"]
    ])

    occupied = 3 - available


    print()
    print("========================================")
    print("        ESP32 DATA RECEIVED")
    print("========================================")

    print(
        "P2:",
        "OCCUPIED"
        if parking_data["slot2"]
        else "FREE"
    )

    print(
        "P3:",
        "OCCUPIED"
        if parking_data["slot3"]
        else "FREE"
    )

    print(
        "P4:",
        "OCCUPIED"
        if parking_data["slot4"]
        else "FREE"
    )

    print("----------------------------------------")

    print("Total slots :", 3)
    print("Available   :", available)
    print("Occupied    :", occupied)

    print("----------------------------------------")

    print(
        "Entry:",
        parking_data["entry_status"]
    )

    print(
        "Barrier:",
        parking_data["barrier"]
    )

    print(
        "Vehicle:",
        parking_data["vehicle_detected"]
    )

    print(
        "RFID:",
        parking_data["last_rfid"]
    )

    print(
        "User:",
        parking_data["last_user"]
    )

    print(
        "Updated:",
        parking_data["last_update"]
    )

    print("========================================")
    print()


    # ========================================================
    # RESPONSE TO ESP32
    # ========================================================

    return jsonify({

        "success": True,

        "message":
            "Parking data updated",

        "total_slots": 3,

        "available_slots":
            available,

        "occupied_slots":
            occupied,

        "timestamp":
            parking_data["last_update"]
    })


# ============================================================
# HEALTH CHECK
# ============================================================

@app.route("/api/health", methods=["GET"])
def health():

    return jsonify({

        "server": "online",

        "esp32_online":
            is_esp32_online(),

        "last_update":
            parking_data["last_update"],

        "time":
            datetime.now().isoformat()
    })


# ============================================================
# RESET
# ============================================================

@app.route("/api/reset", methods=["POST"])
def reset_data():

    parking_data["slot2"] = False
    parking_data["slot3"] = False
    parking_data["slot4"] = False

    parking_data["entry_status"] = "WAITING"
    parking_data["barrier"] = "CLOSED"
    parking_data["vehicle_detected"] = False

    parking_data["last_rfid"] = "NONE"
    parking_data["last_user"] = "NONE"

    parking_data["last_update"] = "Reset"
    parking_data["last_update_timestamp"] = time.time()


    print("Parking data reset.")


    return jsonify({
        "success": True,
        "message": "Parking data reset"
    })


# ============================================================
# RUN SERVER
# ============================================================

if __name__ == "__main__":

    print()
    print("========================================")
    print("       SMART PARKING SYSTEM")
    print("========================================")
    print()
    print("Flask server starting...")
    print()
    print("Parking Slots : P2, P3, P4")
    print("Total Slots   : 3")
    print()
    print("PC IP         :", PC_IP)
    print("Port          :", PORT)
    print()
    print(
        "Dashboard     : "
        f"http://{PC_IP}:{PORT}"
    )
    print()
    print(
        "Update API    : "
        f"http://{PC_IP}:{PORT}/api/update"
    )
    print()
    print(
        "Status API    : "
        f"http://{PC_IP}:{PORT}/api/status"
    )
    print()
    print(
        "Health        : "
        f"http://{PC_IP}:{PORT}/api/health"
    )
    print()
    print("========================================")
    print()


    app.run(
        host="0.0.0.0",
        port=PORT,
        debug=False,
        threaded=True
    )