#include <ArduinoBLE.h>
//#include <Keyboard.h>

#define NUM_KEYS 9
#define LAYERS 2
#define SCAN_PERIOD 5000
#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define KEY_PRESSED 0
#define KEY_UNPRESSED 1

// Whether or not the device will be used as a keyboard
#define KEYBOARD true

BLEDevice keys[NUM_KEYS];
BLECharacteristic keyStates[NUM_KEYS];
int keysConnected = 0;
int layer = 0;

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  Serial.println(DM_CONN_MAX);
  BLE.begin();

  BLE.scanForUuid(UUID);

  int keyCounter = 0;
  unsigned long startMillis = millis();
  Serial.println("Started scan");
  // Wait for either timeout period or all keys found
  while (keyCounter < NUM_KEYS && millis() - startMillis < SCAN_PERIOD) {
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral is already connected
      bool found = false;
      for (int i = 0; i < keyCounter; i++) {
        if (peripheral.address() == keys[i].address()) found = true;
      }

      // If the peripheral was not already previously connected, save it
      if (!found) {
        Serial.print("Found ");
        Serial.print(keyCounter);
        Serial.print(" ");
        Serial.print(peripheral.address());
        Serial.print(" '");
        Serial.print(peripheral.localName());
        Serial.print("' ");
        Serial.print(peripheral.advertisedServiceUuid());
        Serial.println();
        keys[keyCounter] = peripheral;
        keyCounter++;
      }
    }
  }

  BLE.stopScan();
  Serial.println("Finished scan");
  for (int i = 0; i < keyCounter; i++) {
    if (!keys[i].connect()) {
      Serial.println("Error failed to connect");
    }
    keys[i].discoverAttributes();
    BLECharacteristic keyCharacteristic = keys[i].characteristic(UUID);
    if (keyCharacteristic) {
      keyStates[i] = keyCharacteristic;
      keyStates[i].subscribe();
    }
  }
  BLE.setEventHandler(BLEDisconnected, handleDisconnect);
  BLE.setEventHandler(BLEConnected, handleReconnect);
  keysConnected = keyCounter;
  initialized = true;
}

void handleDisconnect(BLEDevice central) {
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  BLE.scanForUuid(UUID);
}

void handleReconnect(BLEDevice central) {
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void loop() {
  uint8_t keyStatuses[NUM_KEYS];
  for (int i = 0; i < keysConnected; i++) {
    if (!keys[i].connected()) {
      reconnectKey(i);
    } else if (keyStates[i].valueUpdated()) {
      uint8_t keyValue;
      keyStates[i].readValue(keyValue);
      if (keyValue <= 1) {
        keyStatuses[i] = keyValue;
        Serial.print("Key ");
        Serial.print(i);
        if (keyValue == KEY_PRESSED) {
          Serial.println(" Pressed");
        }
        else {
          Serial.println(" Unpressed");
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
  
  // Checking if peripheral was previously connected
  for (int i = 0; i < keysConnected; i++) {
    if (i != index && peripheral.address() == keys[i].address()) {
      return;
    }
  }

  BLE.stopScan();
  // Attempt to connect to peripheral
  if (!peripheral.connect()) {
    return;
  }

  keys[index] = peripheral;
  peripheral.discoverAttributes();
  BLECharacteristic keyCharacteristic = peripheral.characteristic(UUID);
  if (keyCharacteristic) {
    keyStates[index] = keyCharacteristic;
    keyStates[index].subscribe();
  }
}
