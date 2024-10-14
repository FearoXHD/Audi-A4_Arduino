#include <BluetoothSerial.h>

BluetoothSerial bleSerial;

String receivedData = "";
bool autoLocking;
bool doorsLocked = true;

const int trunkUnlockPin = 2;
const int doorLockPin = 4;
const int doorUnlockPin = 16;

const int doorsRelayPin1 = 13;
const int doorsRelayPin2 = 12;
const int trunkRelayPin1 = 14;
const int wakePin = 34;

const int motorTimeMs = 50;
const int blockTimeMs = 1500;


void setup() {

    pinMode(trunkUnlockPin, INPUT_PULLUP);
    pinMode(doorUnlockPin, INPUT_PULLUP);
    pinMode(doorLockPin, INPUT_PULLUP);

    pinMode(doorsRelayPin1, OUTPUT);
    pinMode(doorsRelayPin2, OUTPUT);
    pinMode(trunkRelayPin1, OUTPUT);
    pinMode(wakePin, OUTPUT);

    digitalWrite(doorsRelayPin1, HIGH);
    digitalWrite(doorsRelayPin2, HIGH);
    digitalWrite(trunkRelayPin1, HIGH);

    Serial.begin(115200);

    // Starte die Bluetooth-Serial-Kommunikation
    if (!bleSerial.begin("DSD TECH")) {
        Serial.println("Bluetooth initialisierung fehlgeschlagen!");
    } else {
        Serial.println("Bluetooth erfolgreich initialisiert.");
    }

    if (bleSerial.connected()) {
        // Sende Statusnachricht, wenn verbunden
        bleSerial.println("CONNECTED");
    } else {
        bleSerial.println("DISCONNECTED");
    }

 // Array zum Speichern der MAC-Adresse
    uint8_t macAddress[6];
    bleSerial.getBtAddress(macAddress); // MAC-Adresse holen
    
    // MAC-Adresse in lesbares Format konvertieren
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             macAddress[0], macAddress[1], macAddress[2], 
             macAddress[3], macAddress[4], macAddress[5]);
    Serial.println("MAC-Adresse des ESP32: " + String(macStr));

    delay(1000);  // Gebe etwas Zeit für die Bluetooth-Modul-Initialisierung
}

void loop() {
    checkTrunkUnlockPin();
    checkDoorUnlockPin();
    checkDoorLockPin();
    checkSerial();
    checkBle();
}

void checkBle() {
    if (bleSerial.available() > 0) { // Überprüfe, ob Daten verfügbar sind
        char incomingChar = bleSerial.read();
        if (isPrintable(incomingChar)) {
            receivedData += incomingChar;
        }

        // Verarbeite empfangene Daten
        if (receivedData.indexOf("OK+CONN") != -1) {
            receivedData = "";
        } else if (receivedData.indexOf("OK+LOST") != -1) {
            if (autoLocking) {
                lockDoors(); // Schließe die Türen, wenn die Bluetooth-Verbindung verloren geht
            }
            autoLocking = false;
            receivedData = "";
        }

        // Wenn die Daten mit einem Zeilenumbruch enden, verarbeite sie
        if (incomingChar == '\n') {
            receivedData.trim();
            if (receivedData == "ds") {
                bleSerial.println(doorsLocked ? "ld" : "ud");
            } else if (receivedData == "ld") {
                bleSerial.println("ld");
                lockDoors();
            } else if (receivedData == "ud") {
                bleSerial.println("ud");
                unlockDoors();
            } else if (receivedData == "ut") {
                unlockTrunk();
            } else if (receivedData == "al") {
                autoLocking = true;
                if (doorsLocked) {
                    unlockDoors();
                }
            } else if (receivedData == "ald") {
                autoLocking = false;
            }

            receivedData = "";  // Leere die empfangenen Daten nach der Verarbeitung
        }
    }
}

void unlockTrunk() {
    digitalWrite(trunkRelayPin1, LOW);
    delay(motorTimeMs);
    digitalWrite(trunkRelayPin1, HIGH);

    Serial.println("ut");
    delay(blockTimeMs);
}

void unlockDoors() {
    digitalWrite(doorsRelayPin1, LOW);
    digitalWrite(doorsRelayPin2, HIGH);

    delay(motorTimeMs);

    digitalWrite(doorsRelayPin1, HIGH);
    digitalWrite(doorsRelayPin2, HIGH);

    Serial.println("ud");
    doorsLocked = false;
    delay(blockTimeMs);
}

void lockDoors() {
    digitalWrite(doorsRelayPin1, HIGH);
    digitalWrite(doorsRelayPin2, LOW);

    delay(motorTimeMs);

    digitalWrite(doorsRelayPin1, HIGH);
    digitalWrite(doorsRelayPin2, HIGH);

    Serial.println("ld");
    bleSerial.println("Türen Gesperrt");
    doorsLocked = true;
    delay(blockTimeMs);
    
}

void checkSerial() {
    if (Serial.available() > 0) {
        String data = Serial.readStringUntil('\n');

        if (data == "ld") {
            lockDoors();
        } else if (data == "ud") {
            unlockDoors();
        } else if (data == "ut") {
            unlockTrunk();
        }
    }
}

bool lastTrunkUnlockVal;
bool lastDoorUnlockVal;
bool lastDoorLockVal;

void checkTrunkUnlockPin() {
    bool trunkUnlockVal = digitalRead(trunkUnlockPin);

    if (lastTrunkUnlockVal != trunkUnlockVal) {
        lastTrunkUnlockVal = trunkUnlockVal;
        onTrunkUnlockChanged(trunkUnlockVal);
    }
}

void checkDoorUnlockPin() {
    bool doorUnlockVal = digitalRead(doorUnlockPin);

    if (lastDoorUnlockVal != doorUnlockVal) {
        lastDoorUnlockVal = doorUnlockVal;
        onDoorUnlockChanged(doorUnlockVal);
    }
}

void checkDoorLockPin() {
    bool doorLockVal = digitalRead(doorLockPin);

    if (lastDoorLockVal != doorLockVal) {
        lastDoorLockVal = doorLockVal;
        onDoorLockChanged(doorLockVal);
    }
}

void onTrunkUnlockChanged(bool val) {
    if (val == LOW) { // Wenn der Pin LOW ist, entsperre den Kofferraum
        unlockTrunk();
    }
}

void onDoorUnlockChanged(bool val) {
    if (val == HIGH) { // Wenn der Pin HIGH ist, entsperre die Türen
        unlockDoors();
    }
}

void onDoorLockChanged(bool val) {
    if (val == HIGH) { // Wenn der Pin HIGH ist, verriegle die Türen
        lockDoors();
    }
}
