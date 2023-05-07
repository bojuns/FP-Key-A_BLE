// Whether or not the device will be used as a keyboard
#define KEYBOARD true
#include <ArduinoBLE.h>
#ifdef KEYBOARD
#include <Keyboard.h>
#endif

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
{ " ", " ", " ", " ", 
  "5", "6", "7", "8", 
  "9", "A", "B", "C", 
  "D", "E", "RAISE"},       // layer 0
{ "L1", "L2", "L3", "L4", 
  "L5", "L6", "L7", "L8", 
  "L9", "L10", "L11", "L12", 
  "L13", "L14", "L15", "RAISE"}   // layer 1
};

// Whether BLE has been initialized
volatile bool initialized;

void setup() {
  // Starting the serial output
  Serial.begin(9600);
  while (!Serial) ;

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
    pinMode(KEY_PINS[i], INPUT_PULLDOWN);
  }
  // Starting bluetooth
  BLE.begin();
  //BLE.setConnectionInterval(0x000b, 0x0c80); // 7.5 ms minimum, 4 s maximum
  BLE.scanForUuid(UUID);

  // Start looking for keys
  int keyCounter = 0;
  unsigned long startMillis = millis();
  Serial.println("Started scan");
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
  for (int i = 0; i < BLE_KEYS; i++) {
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
        Serial.print("Key ");
        Serial.print(i);
        
        if (keyValue == KEY_PRESSED) {
          Serial.println(" Pressed");
          prev_press[i] = true;
          #ifdef KEYBOARD
          if (strlen(assignments[layer][i]) == 1) {
            Keyboard.press(assignments[layer][i][0]);
          } else if (assignments[layer][i] == "RAISE") {
            layer = 1;
          } else {
            Keyboard.print(assignments[layer][i]);
          }
          #endif
        }
        else if (!prev_press[i]) {
          Serial.println(" Tapped");
          #ifdef KEYBOARD
          if (strlen(assignments[layer][i]) == 1) {
            Keyboard.print(assignments[layer][i][0]);
          } else if (assignments[layer][i] == "RAISE") {
            layer = 1;
          } else {
            Keyboard.print(assignments[layer][i]);
          }
          #endif
        } else {
          Serial.println(" Unpressed");
          prev_press[i] = false;
          #ifdef KEYBOARD
          if (strlen(assignments[layer][i]) == 1) {
            Keyboard.release(assignments[layer][i][0]);
          } else if (assignments[layer][i] == "RAISE") {
            layer = 0;
          }
          #endif
        }
      }
    }
  }
  for (int i = 0; i < PIN_KEYS; i++) {
    if (digitalRead(KEY_PINS[i]) == HIGH && !PIN_PRESSED[i]) {
      PIN_PRESSED[i] = true;
      PIN_RELEASED[i] = false;
      Serial.print("Key ");
      Serial.print(i + BLE_KEYS);
      Serial.println(" Pressed");
      #ifdef KEYBOARD
      if (strlen(assignments[layer][i + BLE_KEYS]) == 1) {
        Keyboard.press(assignments[layer][i + BLE_KEYS][0]);
      } else if (assignments[layer][i + BLE_KEYS] == "RAISE") {
        layer = 1;
      } else {
        Keyboard.print(assignments[layer][i + BLE_KEYS]);
      }
      #endif
    }
    else if (digitalRead(KEY_PINS[i]) == LOW && !PIN_RELEASED[i]) {
      Serial.print("Key ");
      Serial.print(i + BLE_KEYS);
      Serial.println(" Unpressed");
      PIN_PRESSED[i] = false;
      PIN_RELEASED[i] = true;
      #ifdef KEYBOARD
      if (strlen(assignments[layer][i]) == 1) {
        Keyboard.release(assignments[layer][i][0]);
      } else if (assignments[layer][i] == "RAISE") {
        layer = 0;
      }
      #endif
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
      Serial.print("Found index ");
      Serial.println(i);
      new_index = i;
      found = true;
    }
  }

  if (!found) {
    Serial.print("Key address not found in addresses: ");
    Serial.println(peripheral.address());
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

  for (int i = 0; i < BLE_KEYS; i++) {
    if (!keyConnected[i]) {
      Serial.println("Restarting scan");
      BLE.scanForUuid(UUID);
      return;
    }
  }
}