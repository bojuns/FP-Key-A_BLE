#include <ArduinoBLE.h>

#define NUM_KEYS 4
#define LAYERS 2
#define SCAN_PERIOD 5000
#define UUID "1f8a01d0-f945-407f-9c18-498c8f4d3d80"
#define KEY_PRESSED 0
#define KEY_UNPRESSED 1
#define LED_PWR 25

/* Connection parameters used for Peripheral Preferred Connection Parameterss (PPCP) and update request */
bool prev_press[NUM_KEYS];
BLEDevice keys[NUM_KEYS];
BLECharacteristic keyStates[NUM_KEYS];
char keyAddresses[NUM_KEYS][18] = {
  "be:55:26:81:e9:e5", // H
  "7f:94:34:f2:ed:bb", // I
  "8d:28:bd:cc:b1:f7", // J
  "43:b2:41:36:95:14"  // K
};

int key_pins[NUM_KEYS] = {
  6u, 5u, 4u, 3u
};
bool keyConnected[NUM_KEYS];

int keysConnected = 0;

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  // Disable all unused pins
  for (int i = 5; i < 11; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  for (int i = 20; i < 30; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  // Enabling the LEDs
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  // LED initially blue
  digitalWrite(LEDB, HIGH);
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  // Starting the serial output
  Serial.begin(9600);
  // Initializing the key connection counters
  bool keyFound[NUM_KEYS];
  for (int i = 0; i < NUM_KEYS; i++) {
    keyConnected[i] = false;
    keyFound[i] = false;
    prev_press[i] = false;
    pinMode(key_pins[i], OUTPUT);
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
  digitalWrite(LEDB, LOW);
  digitalWrite(LEDR, HIGH);
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
  initialized = true;
  if (failedConnection) {
    Serial.println("Restarting scan");
    BLE.scanForUuid(UUID);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDB, LOW);
  } else {
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDB, LOW);
  }
}

void handleDisconnect(BLEDevice central) {
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, LOW);
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
          prev_press[i] = true;
          digitalWrite(key_pins[i], HIGH);
        }
        else if (!prev_press[i]) {
          Serial.println(" Tapped");
          digitalWrite(key_pins[i], HIGH);
          delay(2);
          digitalWrite(key_pins[i], LOW);
        } 
        else {
          Serial.println(" Unpressed");
          prev_press[i] = false;
          digitalWrite(key_pins[i], LOW);
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
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, LOW);
}