/*
  Battery Monitor

  This example creates a BLE peripheral with the standard battery service and
  level characteristic. The A0 pin is used to calculate the battery level.

  The circuit:
  - Arduino MKR WiFi 1010, Arduino Uno WiFi Rev2 board, Arduino Nano 33 IoT,
    Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board.

  You can use a generic BLE central app, like LightBlue (iOS and Android) or
  nRF Connect (Android), to interact with the services and characteristics
  created in this sketch.

  This example code is in the public domain.
*/

#include <ArduinoBLE.h>


 // BLE Battery Service
BLEService batteryService("180F");

// BLE Battery Level Characteristic
BLEUnsignedCharCharacteristic batteryLevelChar("2A19",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes
BLEStringCharacteristic stringcharacteristic("183E", BLERead | BLEWrite, 31);


// Add BLEEncryption tag to require pairing. This controls the LED.
BLEUnsignedCharCharacteristic secretValue("2a3F", BLERead | BLEWrite | BLEEncryption);

int oldBatteryLevel = 0;  // last battery level reading from analog input
long previousMillis = 0;  // last time the battery level was checked, in ms

void setup() {
  Serial.begin(9600);    // initialize serial communication
  while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected


  Serial.println("Serial connected");

  // IRKs are keys that identify the true owner of a random mac address.
  // Add IRKs of devices you are bonded with.
  BLE.setGetIRKs([](uint8_t* nIRKs, uint8_t** BADDR_TYPES, uint8_t*** BDAddrs, uint8_t*** IRKs){
    // Set to number of devices
    *nIRKs       = 2;

    *BDAddrs     = new uint8_t*[*nIRKs];
    *IRKs        = new uint8_t*[*nIRKs];
    *BADDR_TYPES = new uint8_t[*nIRKs];

    // Set these to the mac and IRK for your bonded devices as printed in the serial console after bonding.
    uint8_t iPhoneMac [6]   =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t iPhoneIRK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t iPadMac[6]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t iPadIRK[16]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };


    (*BADDR_TYPES)[0] = 0;
    (*IRKs)[0] = new uint8_t[16];
    memcpy((*IRKs)[0],iPhoneIRK,16);
    (*BDAddrs)[0] = new uint8_t[6]; 
    memcpy((*BDAddrs)[0], iPhoneMac, 6);


    (*BADDR_TYPES)[1] = 0;
    (*IRKs)[1] = new uint8_t[16];
    memcpy((*IRKs)[1],iPadIRK,16);
    (*BDAddrs)[1] = new uint8_t[6];
    memcpy((*BDAddrs)[1], iPadMac, 6);


    return 1;
  });
  // The LTK is the secret key which is used to encrypt bluetooth traffic
  BLE.setGetLTK([](uint8_t* address, uint8_t* LTK){
    // address is input
    Serial.print("Recieved request for address: ");
    btct.printBytes(address,6);

    // Set these to the MAC and LTK of your devices after bonding.
    uint8_t iPhoneMac [6]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t iPhoneLTK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t iPadMac [6]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t iPadLTK[16]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    

    if(memcmp(iPhoneMac, address, 6)==0){
      memcpy(LTK, iPhoneLTK, 16);
      return 1;
    }else if(memcmp(iPadMac, address, 6)==0){
      memcpy(LTK, iPadLTK, 16);
    }
    return 0;
  });
  BLE.setStoreIRK([](uint8_t* address, uint8_t* IRK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store IRK   : "));
    btct.printBytes(IRK,16);
    return 1;
  });
  BLE.setStoreLTK([](uint8_t* address, uint8_t* LTK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store LTK   : "));
    btct.printBytes(LTK,16);
    return 1;
  });

  while(1){// begin initialization
    if (!BLE.begin()) {
      Serial.println("starting BLE failed!");
      delay(200);
      continue;
    }
    Serial.println("BT init");
    delay(200);
    
    /* Set a local name for the BLE device
       This name will appear in advertising packets
       and can be used by remote devices to identify this BLE device
       The name can be changed but maybe be truncated based on space left in advertisement packet
    */

    BLE.setDeviceName("Arduino");
    BLE.setLocalName("BatteryMonitor");

    BLE.setAdvertisedService(batteryService); // add the service UUID
    batteryService.addCharacteristic(batteryLevelChar); // add the battery level characteristic
    batteryService.addCharacteristic(stringcharacteristic);
    batteryService.addCharacteristic(secretValue);

    BLE.addService(batteryService); // Add the battery service
    batteryLevelChar.writeValue(oldBatteryLevel); // set initial value for this characteristic
    char* stringCharValue = new char[32];
    stringCharValue = "string";
    stringcharacteristic.writeValue(stringCharValue);
    secretValue.writeValue(0);
    
    delay(1000);
  
    /* Start advertising BLE.  It will start continuously transmitting BLE
       advertising packets and will be visible to remote BLE central devices
       until it receives a new connection */
  
    // start advertising
    if(!BLE.advertise()){
      Serial.println("failed to advertise bluetooth.");
      BLE.stopAdvertise();
      delay(500);
    }else{
      Serial.println("advertising...");
      break;
    }
    BLE.end();
    delay(100);
  }
}


void loop() {
  // wait for a BLE central
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's BT address:
    Serial.println(central.address());

    // check the battery level every 200ms
    // while the central is connected:
    while (central.connected()) {
      long currentMillis = millis();
      // if 200ms have passed, check the battery level:
      if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;
        updateBatteryLevel();
        if(secretValue.value()>0){
          digitalWrite(13,HIGH);
        }else{
          digitalWrite(13,LOW);
        }
      }
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void updateBatteryLevel() {
  /* Read the current voltage level on the A0 analog input pin.
     This is used here to simulate the charge level of a battery.
  */
  int battery = analogRead(A0);
  int batteryLevel = map(battery, 0, 1023, 0, 100);

  if (batteryLevel != oldBatteryLevel) {      // if the battery level has changed
    // Serial.print("Battery Level % is now: "); // print it
    // Serial.println(batteryLevel);
    batteryLevelChar.writeValue(batteryLevel);  // and update the battery level characteristic
    oldBatteryLevel = batteryLevel;           // save the level for next comparison
  }
}