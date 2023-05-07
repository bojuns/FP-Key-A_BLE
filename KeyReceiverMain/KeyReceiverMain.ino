// Whether or not the device will be used as a keyboard
#define KEYBOARD true
#include <ArduinoBLE.h>
#ifdef KEYBOARD
#include <Keyboard.h>
#endif
#define SERIAL true

#define BLE_KEYS 7
#define TOTAL_KEYS 16
#define PIN_KEYS 9
#define LAYERS 2
#define SCAN_PERIOD 5000
#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define KEY_PRESSED 0
#define KEY_UNPRESSED 1
// Key to start reading from
#define START_KEY 0
// Key to end reading from
#define END_KEY 6
#define MAX_MACRO_LEN 8
// Pins to read from
const int KEY_PINS[PIN_KEYS] = {
  6u, 5u, 4u, 3u,
  A2, A3, A4, A5, A6
};
bool PIN_PRESSED[PIN_KEYS];
bool PIN_RELEASED[PIN_KEYS];
bool prev_press[BLE_KEYS];
BLEDevice keys[BLE_KEYS];
BLECharacteristic keyStates[BLE_KEYS];
char keyAddresses[BLE_KEYS][18] = {
  "42:ec:0a:24:8e:a1", // Key 1
  "dd:19:14:a7:12:3c", // Key 2
  "c2:a4:ef:0d:bd:56", // Key 3 
  "77:9a:8e:ba:b4:9d", // Key 4
  "c7:90:9f:55:f9:df", // Key 5
  "d5:d4:8d:33:34:58", // Key 6
  "4a:a2:97:83:db:7d", // Key 7
};

bool keyConnected[BLE_KEYS];

int keysConnected = 0;
int layer = 0;
char assignments [LAYERS][TOTAL_KEYS][MAX_MACRO_LEN] = {
{ "a", "b", "c", "d", 
  "e", "f", "g", "h", 
  "i", "j", "k", "l", 
  "m", "n", "o", "RAISE"},    // layer 0
{ "1", "2", "3", "4", 
  "5", "6", "7", "8", 
  "9", "10", "11", "12", 
  "Ctrl", "Alt", "Shift", ""} // layer 1
};

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  // Starting the serial output
  if (SERIAL) {
    Serial.begin(9600);
    while (!Serial) ;
  }
  // Initializing the key connection counters
  bool keyFound[BLE_KEYS];
  for (int i = 0; i < BLE_KEYS; i++) {
    keyConnected[i] = false;
    keyFound[i] = false;
    prev_press[i] = false;
  }
  // Initializing key pressed array
  for (int i = 0; i < PIN_KEYS; i++) {
    PIN_PRESSED[i] = false;
    PIN_RELEASED[i] = true;
    pinMode(KEY_PINS[i], INPUT_PULLDOWN);
  }
  // Starting bluetooth
  BLE.begin();
  BLE.scanForUuid(UUID);
  // Starting the LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // Start looking for keys
  int keyCounter = 0;
  unsigned long startMillis = millis();
  if (SERIAL) Serial.println("Started scan");
  // Wait for either timeout period or all keys found
  while (keyCounter < BLE_KEYS && millis() - startMillis < SCAN_PERIOD) {
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral was found before
      for (int i = 0; i < BLE_KEYS; i++) {
        // If peripheral was not found before, store it
        if (peripheral.address() == keyAddresses[i] && !keyFound[i]) {
          keyFound[i] = true;
          if (SERIAL) {
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
  if (SERIAL) Serial.println("Finished scan");
  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyFound[i]) {
      if (SERIAL) {
        Serial.print("Failed to find ");
        Serial.print(keyAddresses[i]);
        Serial.println();
      }
      failedConnection = true;
      continue;
    }
    if (!keys[i].connect()) {
      if (SERIAL) {
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
      if (SERIAL) {
        Serial.print("Connected: ");
        Serial.print(keys[i].address());
        Serial.println();
      }
      keyConnected[i] = true;
    } else {
      if (SERIAL) {
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
  #ifdef KEYBOARD 
  Keyboard.begin();
  #endif
  initialized = true;
  if (failedConnection) {
    if (SERIAL) Serial.println("Restarting scan");
    BLE.scanForUuid(UUID);
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void handleDisconnect(BLEDevice central) {
  if (SERIAL) Serial.print("Disconnected event, central: ");
  if (SERIAL) Serial.println(central.address());
  digitalWrite(LED_BUILTIN, LOW);
  BLE.scanForUuid(UUID);
}

void handleReconnect(BLEDevice central) {
  if (SERIAL) Serial.print("Connected event, central: ");
  if (SERIAL) Serial.println(central.address());
}
void lookForProgram() {
  if (!Serial.available()) {
    return;
  }
  int index = 0;
  char inputChar;
  char buf[512];
  while (Serial.available() > 0) {
    inputChar = Serial.read();
    buf[index] = inputChar;
    index++;
  }
  Serial.println(buf);
  int layer = 0;
  int key = 0;
  int wordIdx = 0;
  bool escaped = false;
  for (int i = 0; i < index; i++) {
    inputChar = buf[i];
    // Checking for character escape
    if (escaped) {
      escaped = false;
      assignments[layer][key][wordIdx] = inputChar;
      wordIdx++;
    } else {
      // End of line
      if (inputChar == ']') {
        break;
      }
      // Read start of array
      if (inputChar == '[') {
        layer = 0;
        key = 0;
      // Delimiter received
      } else if (inputChar == ',') {
        assignments[layer][key][wordIdx] = '\0';
        wordIdx = 0;
        key++;
        if (key == BLE_KEYS) {
          key = 0;
          layer++;
        }
      } else if (inputChar == '\\') {
        escaped = true;
      } else {
        assignments[layer][key][wordIdx] = inputChar;
        Serial.println(assignments[layer][key][wordIdx]);
        wordIdx++;
      }
    }
  }
  for (int l = 0; l < LAYERS; l++) {
    for (int k = 0; k < BLE_KEYS; k++) {
      Serial.print(assignments[l][k]);
      Serial.print(" ");
    }
    Serial.print("Layer: ");
    Serial.println(l);
  }
}

void loop() {
  lookForProgram();
  uint8_t keyStatuses[BLE_KEYS];
  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyConnected[i] || !keys[i].connected()) {
      reconnectKey(i);
    } else if (keyStates[i] && keyStates[i].valueUpdated()) {
      uint8_t keyValue;
      keyStates[i].readValue(keyValue);
      if (keyValue <= 1) {
        keyStatuses[i] = keyValue;
        if (SERIAL) {
          Serial.print("Key ");
          Serial.print(i);
        }
        if (keyValue == KEY_PRESSED) {
          if (SERIAL) Serial.println(" Pressed");
          prev_press[i] = true;
          #ifdef KEYBOARD
          handleKeyPress(i, false);
          #endif
        }
        else if (!prev_press[i]) {
          Serial.println(" Tapped");
          #ifdef KEYBOARD
          handleKeyPress(i, true);
          #endif
        } else {
          Serial.println(" Unpressed");
          prev_press[i] = false;
          #ifdef KEYBOARD
          handleKeyRelease(i);
          #endif
        }
      }
    }
  }
  for (int i = 0; i < PIN_KEYS; i++) {
    if (digitalRead(KEY_PINS[i]) == HIGH && !PIN_PRESSED[i]) {
      PIN_PRESSED[i] = true;
      PIN_RELEASED[i] = false;
      if (SERIAL) {
        Serial.print("Key ");
        Serial.print(i + BLE_KEYS);
        Serial.println(" Pressed");
      }
      #ifdef KEYBOARD
      handleKeyPress(i + BLE_KEYS, false);
      #endif
    }
    else if (digitalRead(KEY_PINS[i]) == LOW && !PIN_RELEASED[i]) {
      if (SERIAL) {
        Serial.print("Key ");
        Serial.print(i + BLE_KEYS);
        Serial.println(" Unpressed");
      }
      PIN_PRESSED[i] = false;
      PIN_RELEASED[i] = true;
      #ifdef KEYBOARD
      handleKeyRelease(i + BLE_KEYS);
      #endif
    }
  }
}
#ifdef KEYBOARD
void handleKeyPress(uint8_t index, bool tap) {
  char* assignment = assignments[layer][index];
  char len = strlen(assignment);
  // Single key pressed
  if (len == 1) {
    if (tap) {
      Keyboard.print(assignment[0]);
    } else {
      Keyboard.press(assignment[0]);
    }
    return;
  }

  // Raise pressed
  if (strncmp(assignment, "RAISE", len) == 0) {
    layer = 1;
    return;
  }

  // shift pressed
  if (strncmp(assignment, "Shift", len) == 0) {
    Keyboard.press(KEY_LEFT_SHIFT);
    return;
  }

  // Control pressed
  if (strncmp(assignment, "Ctrl", len) == 0) {
    Keyboard.press(KEY_LEFT_CTRL);
    return;
  }

  // Alt pressed
  if (strncmp(assignment, "Alt", len) == 0) {
    Keyboard.press(KEY_LEFT_ALT);
    return;
  }

  // Command released
  if (strncmp(assignment, "Cmd", len) == 0) {
    Keyboard.press(KEY_LEFT_GUI);
    return;
  }

  // Macro
  Keyboard.print(assignment);
}
void handleKeyRelease(uint8_t index) {
  char* assignment = assignments[layer][index];
  char len = strlen(assignment);
  // Single key released
  if (len == 1) {
    Keyboard.release(assignment[0]);
    return;
  }

  // Raise released
  if (strncmp(assignment, "RAISE", len) == 0) {
    layer = 0;
    return;
  }

  // shift released
  if (strncmp(assignment, "Shift", len) == 0) {
    Keyboard.release(KEY_LEFT_SHIFT);
    return;
  }

  // Control released
  if (strncmp(assignment, "Ctrl", len) == 0) {
    Keyboard.release(KEY_LEFT_CTRL);
    return;
  }

  // Alt released
  if (strncmp(assignment, "Alt", len) == 0) {
    Keyboard.release(KEY_LEFT_ALT);
    return;
  }

  // Command released
  if (strncmp(assignment, "Cmd", len) == 0) {
    Keyboard.release(KEY_LEFT_GUI);
    return;
  }
}
#endif
void reconnectKey(int index) {
  BLEDevice peripheral = BLE.available();
  // Checking peripheral validity
  if (!(peripheral && peripheral.localName() == "Key")) {
    return;
  }
  Keyboard.releaseAll();
  // Checking if peripheral found was the one previously connected
  bool found = false;
  int new_index = 0;
  for (int i = 0; i < BLE_KEYS; i++) {
    if (peripheral.address() == keyAddresses[i]) {
      if (SERIAL) {
        Serial.print("Found index ");
        Serial.println(i);
      }
      new_index = i;
      found = true;
    }
  }

  if (!found) {
    if (SERIAL) {
      Serial.print("Key address not found in addresses: ");
      Serial.println(peripheral.address());
    }
    return;
  }
  // Attempt to connect to peripheral
  BLE.stopScan();
  if (!peripheral.connect()) {
    if (SERIAL) Serial.println("Unable to connect to peripheral, restarting scan");
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
    if (SERIAL) Serial.println("Failed to discover characteristics");
  }

  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyConnected[i]) {
      if (SERIAL) Serial.println("Restarting scan");
      BLE.scanForUuid(UUID);
      return;
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);
}