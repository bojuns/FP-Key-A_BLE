#include <ArduinoBLE.h>
//#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214" // Main
#define UUID "00ea5405-8ade-4d59-acf2-4e74ec6d3533" // Board 1
//#define UUID "2cd03a4f-9578-440f-b59c-105255aadd44" // Board 2
#define DEBUG false
#define SLEEP_TIME 10*60*1000 // Idle for 10 minutes before going to sleep

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
  // If timeout, put the device to sleep
  if (millis() - prev_interrupt_time > SLEEP_TIME) {
    nrf_gpio_cfg_sense_input(keyPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    NRF_POWER->SYSTEMOFF = 1;
  }
  delay(50);
}

// Upon key press, send data after debouncing
void keyPressHandler() {
  // Debouncing the switch
  unsigned int curr_time = millis();
  if (curr_time - prev_interrupt_time > (unsigned int) 8) {
    char keyValue = !keyCharacteristic.value();
    keyCharacteristic.writeValue(keyValue);
  }
  prev_interrupt_time = curr_time;
}