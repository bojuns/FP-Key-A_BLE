#include <ArduinoBLE.h>
 
BLEService ledService("2c66a98d-5732-42c3-85a2-16ed85b07d4d"); // Bluetooth® Low Energy LED Service
 
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("2c66a98d-5732-42c3-85a2-16ed85b07d4d", BLERead | BLEWrite);
 
const int ledPin = LED_BUILTIN; // pin to use for the LED
 
void setup() {
  Serial.begin(9600);
  while (!Serial);
 
  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);
 
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("Starting BLE Failed");
    while (1);
  }
 
  // set advertised local name and service UUID:
  BLE.setLocalName("Key1");
  BLE.setAdvertisedService(ledService);
 
  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);
 
  // add service
  BLE.addService(ledService);
 
  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);
 
  // start advertising
  BLE.advertise();
 
  // print address
  Serial.print("Address: ");
  Serial.println(BLE.address());
  Serial.println("Key Receiver Ready");
}
 
void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();
 
  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
 
    // while the central is still connected to peripheral:
    while (central.connected()) {
      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      if (switchCharacteristic.written()) {
        byte val = 0;
        switchCharacteristic.readValue(val);
        Serial.print("Signal received");
        Serial.println(val);
        if (val) {   // any value other than 0
          digitalWrite(ledPin, HIGH);         // will turn the LED on
        } else {                              // a 0 value
          Serial.println(F("Key Release Received"));
          digitalWrite(ledPin, LOW);          // will turn the LED off
        }
      }
    }

    // when the central disconnects, print it out:
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
