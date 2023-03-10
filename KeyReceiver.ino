#include <ArduinoBLE.h>

#define NUM_KEYS 1
#define SCAN_PERIOD 5000
#define UUID "19B10010-E8F2-537E-4F6C-D104768A1214"

BLEDevice keys[NUM_KEYS];
BLECharacteristic keyStates[NUM_KEYS];
int keysConnected = 0;

void setup()
{
  Serial.begin( 9600 );
  while (!Serial);

  BLE.begin();
  
  BLE.scanForUuid(UUID);

  int keyCounter = 0;
  unsigned long startMillis = millis();
  Serial.println("Started scan");
  // Wait for either timeout period or all keys found
  while (keyCounter < NUM_KEYS) {
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral is already connected
      bool found = false;
      for (int i = 0; i < keyCounter; i++) {
        if (peripheral.address() == keys[i].address()) found = true;
      }

      // If the peripheral was not already previously connected, save it
      if (!found) {
        Serial.print("Found ");
        Serial.print(keyCounter);
        Serial.print(" ");
        Serial.print(peripheral.address());
        Serial.print(" '");
        Serial.print(peripheral.localName());
        Serial.print("' ");
        Serial.print(peripheral.advertisedServiceUuid());
        Serial.println();
        keys[keyCounter] = peripheral;
        keyCounter++;
      }
    }
  }
  
  BLE.stopScan();
  Serial.println("Finished scan");
  for ( int i = 0; i < keyCounter; i++ )
  {
            if (!keys[i].connect()) {
          Serial.println("Error failed to connect");
        }
    keys[i].discoverAttributes();
    BLECharacteristic keyCharacteristic = keys[i].characteristic(UUID);
    if (keyCharacteristic)
    {
      keyStates[i] = keyCharacteristic;
      keyStates[i].subscribe();
    }
  }
  keysConnected = keyCounter;
}

void loop()
{
  uint8_t keyStatuses[NUM_KEYS];
  bool showStatuses = false;
  for ( int i = 0; i < keysConnected; i++ )
  {
    if (!keys[i].connected()) {
      handleReconnect(i);
    }
    if (keyStates[i].valueUpdated())
    {
      showStatuses = true;
      uint8_t keyValue;
      keyStates[i].readValue(keyValue);
      keyStatuses[i] = keyValue;
    }
  }
  
  if (showStatuses)
  {
    for (int i = 0; i < keysConnected; i++)
    {
      Serial.print(keyStatuses[i]);
      Serial.print( "," ); 
    }
    Serial.print( "\n" );
  }
}
void handleReconnect(int index) {
  Serial.print("Disconnected: ");
  Serial.println(keys[index].address());
  BLE.scanForUuid(UUID);
  while (true) {
    BLEDevice peripheral = BLE.available();
    // If a peripheral is found and is a key
    if (peripheral && peripheral.localName() == "Key") {
      // Determining if the peripheral is already connected
      bool found = false;
      for (int i = 0; i < keysConnected; i++) {
        if (i != index && peripheral.address() == keys[i].address()) found = true;
      }
      Serial.print("Found ");
      Serial.print(index);
      Serial.print(" ");
      Serial.print(peripheral.address());
      Serial.print(" '");
      Serial.print(peripheral.localName());
      Serial.print("' ");
      Serial.print(peripheral.advertisedServiceUuid());
      Serial.println();
      // If the peripheral was not already previously connected, save it
      if (!found) {
        Serial.println("Connected");
        if (!peripheral.connect()) {
          Serial.println("Error failed to connect");
        }
        keys[index] = peripheral;
        peripheral.discoverAttributes();
        BLECharacteristic keyCharacteristic = peripheral.characteristic(UUID);
        if (keyCharacteristic)
        {
          keyStates[index] = keyCharacteristic;
          keyStates[index].subscribe();
        }
        BLE.stopScan();
        while (!peripheral.connected());
        return;
      }
    }
  }
}
