// Whether or not the device will be used as a keyboard
#define KEYBOARD true
#include <ArduinoBLE.h>

#define BLE_KEYS 5
#define SCAN_PERIOD 5000
#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define KEY_PRESSED 0
#define KEY_UNPRESSED 1
// Pins to write to
const uint8_t KEY_PINS[BLE_KEYS] = {
  A1,A2,A3,A4,A5
};
bool PIN_PRESSED[BLE_KEYS];
bool canSerial;
BLEDevice keys[BLE_KEYS];
BLECharacteristic keyStates[BLE_KEYS];
char keyAddresses[BLE_KEYS][18] = {
  "57:53:ad:c9:88:41",
  "8d:28:bd:cc:b1:f7",
  "4d:3f:3d:1c:f1:79",
  "65:fd:1c:a3:49:df",
  "6a:49:8d:cc:c8:39"
};

bool keyConnected[BLE_KEYS];

int keysConnected = 0;

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  canSerial = false;
  // Starting the serial output
  Serial.begin(9600);
  // Initializing the key connection counters
  bool keyFound[BLE_KEYS];
  for (int i = 0; i < BLE_KEYS; i++) {
    keyConnected[i] = false;
    keyFound[i] = false;
  }
  // Initializing key pressed array
  for (int i = 0; i < BLE_KEYS; i++) {
    PIN_PRESSED[i] = false;
  }
  // Starting bluetooth
  BLE.begin();
  //BLE.setConnectionInterval(0x000b, 0x0c80); // 7.5 ms minimum, 4 s maximum
  BLE.scanForUuid(UUID);

  // Start looking for keys
  int keyCounter = 0;
  unsigned long startMillis = millis();
  if (canSerial) Serial.println("Started scan");
  // Wait for either timeout period or all keys found
  while (keyCounter < BLE_KEYS && millis() - startMillis < SCAN_PERIOD) {
    if (Serial && !canSerial) {
      canSerial = true;
    }
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral was found before
      for (int i = 0; i < BLE_KEYS; i++) {
        // If peripheral was not found before, store it
        if (peripheral.address() == keyAddresses[i] && !keyFound[i]) {
          keyFound[i] = true;
          if (canSerial) {
            Serial.print("Found key ");
            Serial.print(i);
            Serial.println();
          }
          keys[i] = peripheral;
          keyCounter++;
        }
      }
    }
  }

  // Stop the scan and attempt to connect to found peripherals
  BLE.stopScan();
  bool failedConnection = false;
  if (canSerial) Serial.println("Finished scan");
  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyFound[i]) {
      if (canSerial) {
        Serial.print("Failed to find ");
        Serial.print(keyAddresses[i]);
        Serial.println();
      }
      failedConnection = true;
      continue;
    }
    if (!keys[i].connect()) {
      if (canSerial) {
        Serial.print("Failed to connect ");
        Serial.print(keys[i].address());
        Serial.println();
      }
      keys[i].disconnect();
      failedConnection = true;
      continue;
    }
    keys[i].discoverAttributes();
    BLECharacteristic keyCharacteristic = keys[i].characteristic(UUID);
    if (keyCharacteristic) {
      keyStates[i] = keyCharacteristic;
      keyStates[i].subscribe();
      if (canSerial) {
        Serial.print("Connected: ");
        Serial.print(keys[i].address());
        Serial.println();
      }
      keyConnected[i] = true;
    } else {
      if (canSerial) {
        Serial.print("Failed to discover attributes: ");
        Serial.print(keys[i].address());
        Serial.println();
      }
      failedConnection = true;
      keys[i].disconnect();
    }
  }
  
  BLE.setEventHandler(BLEDisconnected, handleDisconnect);
  BLE.setEventHandler(BLEConnected, handleReconnect);
  keysConnected = keyCounter;
  initialized = true;
  if (failedConnection) {
    if (canSerial) Serial.println("Restarting scan");
    BLE.scanForUuid(UUID);
  }
}

void handleDisconnect(BLEDevice central) {
  if (canSerial) {
    Serial.print("Disconnected event, central: ");
    Serial.println(central.address());
  }
  BLE.scanForUuid(UUID);
}

void handleReconnect(BLEDevice central) {
  if (canSerial) {
    Serial.print("Connected event, central: ");
    Serial.println(central.address());
  }
}
void loop() {
  uint8_t keyStatuses[BLE_KEYS];
  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyConnected[i] || !keys[i].connected()) {
      reconnectKey(i);
    } else if (keyStates[i] && keyStates[i].valueUpdated()) {
      uint8_t keyValue;
      keyStates[i].readValue(keyValue);
      if (keyValue <= 1) {
        keyStatuses[i] = keyValue;
        if (canSerial) {
          Serial.print("Key ");
          Serial.print(i);
        }
        if (keyValue == KEY_PRESSED) {
          if (canSerial) Serial.println(" Pressed");
          digitalWrite(KEY_PINS[i], HIGH);
        }
        else {
          if (canSerial) Serial.println(" Released");
          digitalWrite(KEY_PINS[i], LOW);
        }
      }
    }
  }
}

void reconnectKey(int index) {
  BLEDevice peripheral = BLE.available();
  // Checking peripheral validity
  if (!(peripheral && peripheral.localName() == "Key")) {
    return;
  }
  
  // Checking if peripheral found was the one previously connected
  bool found = false;
  int new_index = 0;
  for (int i = 0; i < BLE_KEYS; i++) {
    if (peripheral.address() == keyAddresses[i]) {
      if (canSerial) {
        Serial.print("Found index ");
        Serial.println(i);
      }
      new_index = i;
      found = true;
    }
  }

  if (!found) {
    if (canSerial) {
      Serial.print("Key address not found in addresses: ");
      Serial.print(peripheral.address());
    }
    return;
  }
  // Attempt to connect to peripheral
  BLE.stopScan();
  if (!peripheral.connect()) {
    if (canSerial) Serial.println("Unable to connect to peripheral, restarting scan");
    BLE.scanForUuid(UUID);
    return;
  }

  keys[new_index] = peripheral;
  peripheral.discoverAttributes();
  BLECharacteristic keyCharacteristic = peripheral.characteristic(UUID);
  if (keyCharacteristic) {
    keyStates[new_index] = keyCharacteristic;
    keyStates[new_index].subscribe();
    keyConnected[new_index] = true;
  } else {
    if (canSerial) Serial.println("Failed to discover characteristics");
  }

  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyConnected[i]) {
      if (canSerial) Serial.println("Restarting scan");
      BLE.scanForUuid(UUID);
      return;
    }
  }
}