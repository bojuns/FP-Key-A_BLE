#include <ArduinoBLE.h>
#ifdef KEYBOARD
#include <Keyboard.h>
#endif

#define NUM_KEYS 8
#define LAYERS 2
#define SCAN_PERIOD 5000
#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define KEY_PRESSED 0
#define KEY_UNPRESSED 1

// Whether or not the device will be used as a keyboard
//#define KEYBOARD false

/* Connection parameters used for Peripheral Preferred Connection Parameterss (PPCP) and update request */
#define DEFAULT_MIN_CONN_INTERVAL MSEC_TO_UNITS(5, UNIT_0_625_MS)
#define DEFAULT_MAX_CONN_INTERVAL MSEC_TO_UNITS(10, UNIT_0_625_MS)

#define MIN_CONN_INTERVAL 0x0005
#define MAX_CONN_INTERVAL 0x000a

BLEDevice keys[NUM_KEYS];
BLECharacteristic keyStates[NUM_KEYS];
char keyAddresses[NUM_KEYS][18] = {
  "42:ec:0a:24:8e:a1", // Key 9
  "dd:19:14:a7:12:3c", // Key 1
  "c2:a4:ef:0d:bd:56", // Key 2 
  //"43:b2:41:36:95:14", // Key 3
  "77:9a:8e:ba:b4:9d", // Key 4
  "c7:90:9f:55:f9:df", // Key 5
  "d5:d4:8d:33:34:58", // Key 6
  "4a:a2:97:83:db:7d", // Key 7
  "be:55:26:81:e9:e5" // Key 8
};

bool keyConnected[NUM_KEYS];

int keysConnected = 0;
int layer = 0;
char assignments [LAYERS][NUM_KEYS] = {
// {'a','KEY_LEFT_SHIFT'},  // layer 0
// {'c','KEY_LEFT_CTRL'}   // layer 1
};

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  // Starting the serial output
  Serial.begin(9600);
  while (!Serial) ;

  // Initializing the key connection counters
  bool keyFound[NUM_KEYS];
  for (int i = 0; i < NUM_KEYS; i++) {
    keyConnected[i] = false;
    keyFound[i] = false;
  }
  // Starting bluetooth
  BLE.begin();
  BLE.setConnectionInterval(0x000b, 0x0c80); // 7.5 ms minimum, 4 s maximum
  BLE.scanForUuid(UUID);

  // Start looking for keys
  int keyCounter = 0;
  unsigned long startMillis = millis();
  Serial.println("Started scan");
  // Wait for either timeout period or all keys found
  while (keyCounter < NUM_KEYS && millis() - startMillis < SCAN_PERIOD) {
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral was found before
      for (int i = 0; i < NUM_KEYS; i++) {
        // If peripheral was not found before, store it
        if (peripheral.address() == keyAddresses[i] && !keyFound[i]) {
          keyFound[i] = true;
          Serial.print("Found key ");
          Serial.print(i);
          Serial.println();
          keys[i] = peripheral;
          keyCounter++;
        }
      }
    }
  }

  // Stop the scan and attempt to connect to found peripherals
  BLE.stopScan();
  bool failedConnection = false;
  Serial.println("Finished scan");
  for (int i = 0; i < NUM_KEYS; i++) {
    if (!keyFound[i]) {
      Serial.print("Failed to find ");
      Serial.print(keyAddresses[i]);
      Serial.println();
      failedConnection = true;
      continue;
    }
    if (!keys[i].connect()) {
      Serial.print("Failed to connect ");
      Serial.print(keys[i].address());
      Serial.println();
      keys[i].disconnect();
      failedConnection = true;
      continue;
    }
    keys[i].discoverAttributes();
    BLECharacteristic keyCharacteristic = keys[i].characteristic(UUID);
    if (keyCharacteristic) {
      keyStates[i] = keyCharacteristic;
      keyStates[i].subscribe();
      Serial.print("Connected: ");
      Serial.print(keys[i].address());
      Serial.println();
      keyConnected[i] = true;
    } else {
      Serial.print("Failed to discover attributes: ");
      Serial.print(keys[i].address());
      Serial.println();
      failedConnection = true;
      keys[i].disconnect();
    }
  }
  
  BLE.setEventHandler(BLEDisconnected, handleDisconnect);
  BLE.setEventHandler(BLEConnected, handleReconnect);
  keysConnected = keyCounter;
  #ifdef KEYBOARD 
  Keyboard.begin();
  #endif
  initialized = true;
  if (failedConnection) {
    Serial.println("Restarting scan");
    BLE.scanForUuid(UUID);
  }
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
  for (int i = 0; i < NUM_KEYS; i++) {
    if (!keyConnected[i] || !keys[i].connected()) {
      reconnectKey(i);
    } else if (keyStates[i] && keyStates[i].valueUpdated()) {
      uint8_t keyValue;
      keyStates[i].readValue(keyValue);
      if (keyValue <= 1) {
        keyStatuses[i] = keyValue;
        Serial.print("Key ");
        Serial.print(i);
        if (keyValue == KEY_PRESSED) {
          Serial.println(" Pressed");
          #ifdef KEYBOARD
          Keyboard.press(assignments[layer][i]);
          #endif
        }
        else {
          Serial.println(" Unpressed");
          #ifdef KEYBOARD
          Keyboard.release(assignments[layer][i]);
          #endif
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
  for (int i = 0; i < NUM_KEYS; i++) {
    if (peripheral.address() == keyAddresses[i]) {
      Serial.print("Found index ");
      Serial.println(i);
      new_index = i;
      found = true;
    }
  }

  if (!found) {
    Serial.print("Key address not found in addresses: ");
    Serial.print(peripheral.address());
    return;
  }
  // Attempt to connect to peripheral
  BLE.stopScan();
  if (!peripheral.connect()) {
    Serial.println("Unable to connect to peripheral, restarting scan");
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
    Serial.println("Failed to discover characteristics");
  }

  for (int i = 0; i < NUM_KEYS; i++) {
    if (!keyConnected[i]) {
      Serial.println("Restarting scan");
      BLE.scanForUuid(UUID);
      return;
    }
  }
}
