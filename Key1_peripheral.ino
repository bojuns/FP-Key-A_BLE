#include <ArduinoBLE.h>
#include <Wire.h>

// Keyswitch Variables
const int keyPin = D1;
int oldKeyState = HIGH;

// BLE components
char UUID[] = "afc29fc9-c858-4fb6-92be-96d8eae7066f";
BLEService key_service(UUID);
BLEByteCharacteristic key_state_characteristic(UUID, BLERead | BLEWrite);

// Key 1 variables:
byte key_mask = 0b00000001;
byte key_state = 0b00000000;
byte old_key_state = 0b00000000;

// Enable Serial debugging:
bool debug = true;

// Connection Info
char current_name[] = "Key1";
char receive_name[] = "Key2";
char send_to_name[] = "KeyReceiver";
char receive_UUID[] = "19b10001-e8f2-537e-4f6c-d104768a1214";
char send_to_UUID[] = "2c66a98d-5732-42c3-85a2-16ed85b07d4d";

void setup() {
  // Serial output for debugging
  if (debug) {
    Serial.begin(9600);
    while (!Serial);
  }

  // Keyswitch is input
  pinMode(keyPin, INPUT_PULLUP);

  // Initialize BLE:
  if (!BLE.begin()) {
    if (debug) Serial.println("Starting BLE failed");
    while (1);
  }

  // Set advertised local name and service UUID
  BLE.setLocalName(current_name);
  BLE.setAdvertisedService(key_service);

  // Adding key state characteristic to service
  key_service.addCharacteristic(key_state_characteristic);

  // Adding service to BLE
  BLE.addService(key_service);

  // Set initial value for characteristic to be unpressed
  key_state_characteristic.writeValue(old_key_state);

  // Setting handlers for connected, disconnected from previous key peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // Handler for if previous peripheral changes key state
  key_state_characteristic.setEventHandler(BLEWritten, switchCharacteristicWritten);

  // set the discovered peripheral event handle - needed as a central and mores specifically scaling
  BLE.setEventHandler(BLEDiscovered, bleCentralDiscoverHandler);
  
  // Start advertising:
  BLE.advertise();
  if (debug) {
    Serial.println("Started advertise with address");
    Serial.println(BLE.address());
  }
  
  // Start searching for previous key in the chain
  BLE.scanForName(send_to_name);
}
void bleCentralDiscoverHandler(BLEDevice peripheral) {
  if (peripheral) {
    if (peripheral.localName() != send_to_name) {
      return;
    }

    // Stop scanning and run key listener
    BLE.stopScan();
    system_control(peripheral);
 
    // Disconnected from receiver, start scan again
    BLE.scanForName(send_to_name);
  }
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
  BLECharacteristic keyCharacteristic = controller.characteristic(send_to_UUID);

  // If characteristic invalid, disconnect and try again
  if (!keyCharacteristic) {
    controller.disconnect();
    return;
  } 
  // If characteristic not writable, disconnect and try again
  else if (!keyCharacteristic.canWrite()) {
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
        keyCharacteristic.writeValue((byte)(key_state << 1 | 1));
      } else {
        // Key is released, write 0x00 to turn the LED off on controller
        keyCharacteristic.writeValue((byte)(key_state << 1));
      }
    }
  }
}

void blePeripheralConnectHandler(BLEDevice central) {
  // Central connected event handler
  if (debug) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
  }
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // Central disconnected event handler
  if (debug) {
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void switchCharacteristicWritten(BLEDevice central, BLECharacteristic switchcharacteristic) {
  byte switch_states = 0;
  switchcharacteristic.readValue(switch_states);
  if (debug) {
    Serial.print("Received packet: ");
    Serial.println((byte) switch_states);
  }
  key_state = (byte) switch_states;
}

void loop() {
  BLE.poll();
}
