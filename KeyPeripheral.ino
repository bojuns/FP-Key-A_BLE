#include <ArduinoBLE.h>

#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define DEBUG false

const int keyPin = D1;
BLEService keyService(UUID); // create service
// create button characteristic and allow remote device to get notifications
BLEByteCharacteristic keyCharacteristic(UUID, BLERead | BLENotify);

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial);
  }
  
  pinMode(keyPin, INPUT_PULLUP); // use button pin as an input

  // begin initialization
  if (!BLE.begin()) {
    if (DEBUG) Serial.println("starting BluetoothÂ® Low Energy module failed!");
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

  keyCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();
  
  if (DEBUG) Serial.println("BluetoothÂ® device active, waiting for connections...");
}

void loop() {
  BLEDevice receiver = BLE.central();
  
  if (receiver) {
    BLE.stopAdvertise();
    while (receiver.connected()) {
      // read the current button pin state
      char keyValue = digitalRead(keyPin);
      
      // has the value changed since the last read
      bool keyChanged = keyCharacteristic.value() != keyValue;
      
      for (int i = 0; i < 10; i++) {
        if (digitalRead(keyPin) != keyValue) {
          keyChanged = false;
          break;
        }
      }
      if (keyChanged) {
        // Key state changed, update characteristic
        keyCharacteristic.writeValue(keyValue);
      }
    }
  }
  BLE.advertise();
}
