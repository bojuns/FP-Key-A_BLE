#include <ArduinoBLE.h>

#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define DEBUG true

const int outPin = D8;
const int keyPin = D9;
BLEService keyService(UUID); // create service
// create button characteristic and allow remote device to get notifications
BLEByteCharacteristic keyCharacteristic(UUID, BLERead | BLENotify);

// Interrupt timing
unsigned int prev_interrupt_time;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial);
  }
  
  // Setting the pins as pulldown to reduce power consumption
  for (int i = 0; i < 8; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  pinMode(keyPin, INPUT_PULLUP);  // Button Pin reads 
  pinMode(outPin, OUTPUT); // Output pin provides HIGH logic value
  digitalWrite(outPin, LOW);

  // begin initialization
  if (!BLE.begin()) {
    if (DEBUG) Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("Key");
  
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(keyService);

  // Add key characteristic to key service
  keyService.addCharacteristic(keyCharacteristic);

  // add the service
  BLE.addService(keyService);

  keyCharacteristic.writeValue(1);

  // start advertising
  BLE.advertise();
   
  if (DEBUG) Serial.println("Bluetooth® $device active, waiting for connections...");

  // Interrupt with key press
  attachInterrupt(keyPin, keyPressHandler, CHANGE);

  // Interrupt if connected to central
  BLE.setEventHandler(BLEConnected, handleConnect);
  BLE.setEventHandler(BLEDisconnected, handleDisconnect);

  // Set previous interrupt time
  prev_interrupt_time = 0;
}

// Upon connecting to central, stop advertising to save power
void handleConnect(BLEDevice central) {
  BLE.stopAdvertise();
}

// Upon disconnection, restart advertising
void handleDisconnect(BLEDevice central) {
  BLE.advertise();
}

// Sleep most of the time to save power
void loop() {
  BLEDevice receiver = BLE.central();
  delay(100);
}

// Upon key press, send data after debouncing
void keyPressHandler() {
  // Debouncing the switch
  unsigned int curr_time = millis();
  if (curr_time - prev_interrupt_time > (unsigned int) 7) {
    char keyValue = !keyCharacteristic.value();
    keyCharacteristic.writeValue(keyValue);
  }
  prev_interrupt_time = curr_time;
}
