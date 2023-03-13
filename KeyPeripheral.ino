#include <ArduinoBLE.h>

#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define DEBUG true

const int keyPin = D1;
BLEService keyService(UUID);  // create service
// create button characteristic and allow remote device to get notifications
BLEByteCharacteristic keyCharacteristic(UUID, BLERead | BLENotify);
volatile bool flag;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial)
      ;
  }

  pinMode(keyPin, INPUT_PULLDOWN);  // use button pin as an input
  flag = false;
  attachInterrupt(keyPin, keyPressHandler, CHANGE);

  // begin initialization
  if (!BLE.begin()) {
    if (DEBUG) Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
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

  if (DEBUG) Serial.println("Bluetooth® device active, waiting for connections...");
}

void loop() {
  // poll for Bluetooth® Low Energy events
  BLE.poll();
  if (flag) {
    Serial.println("Flag true");
    flag = false;
  }
}

void keyPressHandler() {
  flag = true;
  // // read the current button pin state
  // char keyValue = digitalRead(keyPin);

  // // has the value changed since the last read
  // bool keyChanged = keyCharacteristic.value() != keyValue;

  // if (keyChanged) {
  //   // Key state changed, update characteristic
  //   keyCharacteristic.writeValue(keyValue);
  // }

  // if (DEBUG && (keyCharacteristic.written() || keyChanged)) {
  //   if (keyCharacteristic.value()) {
  //     Serial.println("key pressed");
  //   } else {
  //     Serial.println("key released");
  //   }
  // }
}
