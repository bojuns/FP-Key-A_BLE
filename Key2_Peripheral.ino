#include <ArduinoBLE.h>
#include <Wire.h>
 
// Keyswitch Variables
const int keyPin = D1;
int oldKeyState = HIGH;

void setup() {
  // Keyswitch is input, LED is output
  pinMode(keyPin, INPUT_PULLUP);
  
  // Initialize BLE
  BLE.begin();

  // Start scanning for server
  BLE.scanForName("Key1");
}
 
void loop() {
  // Check if server has been discovered
  BLEDevice controller = BLE.available();
  if (controller) {
    if (controller.localName() != "Key1") {
      return;
    }

    // Stop scanning and run key listener
    BLE.stopScan();
    system_control(controller);
 
    // Disconnected from receiver, start scan again
    BLE.scanForName("Key1");
  }
  delay(100);
}
 
void system_control(BLEDevice controller) {
  // Connect to key controller
  if (!controller.connect()) {
    return;
  }
 
  // Find controller attributes
  if (!controller.discoverAttributes()) {
    controller.disconnect();
    return;
  }
 
  // retrieve the LED characteristic
  BLECharacteristic ledCharacteristic = controller.characteristic("afc29fc9-c858-4fb6-92be-96d8eae7066f");

  // If characteristic invalid, disconnect and try again
  if (!ledCharacteristic) {
    controller.disconnect();
    return;
  } 
  // If characteristic not writable, disconnect and try again
  else if (!ledCharacteristic.canWrite()) {
    controller.disconnect();
    return;
  }
 
  while (controller.connected()) {
    // Read key state while connected
    int keyState = digitalRead(keyPin);
 
    if (oldKeyState != keyState) {
      // button changed
      oldKeyState = keyState;
 
      if (keyState) {
        // Key is pressed, write 0x01 to turn the LED on on controller
        ledCharacteristic.writeValue((byte)0x01);
      } else {
        // Key is released, write 0x00 to turn the LED off on controller
        ledCharacteristic.writeValue((byte)0x00);
      }
    }
  }
}
