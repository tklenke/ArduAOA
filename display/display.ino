/*
  Create a BLE client that will receive pressure, temperature and battery voltage notifications from an AoA probe
  and display that data along with data from the local microcontroller on a small OLED screen and a NeoPixel 8x
  LED strip.
  
  BLE information based on 
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
*/
/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"

//set VERBOSE to 1 if you want more print statements sent to the Serial port.
#define VERBOSE 1
//#define VERSOSE 0

// The remote service we wish to connect to.
static BLEUUID serviceUUID("8aaeec2c-7f43-11ee-b962-0242ac120002");
// The characteristic of the remote service we are interested in.

static BLEUUID    charBatteryVoltageUUID("8aaeee98-7f43-11ee-b962-0242ac120002");
static BLEUUID    charTempUUID("8aaeefba-7f43-11ee-b962-0242ac120002");
static BLEUUID    charPressureFwd("8aaef0c8-7f43-11ee-b962-0242ac120002");
static BLEUUID    charPressure45("8aaef244-7f43-11ee-b962-0242ac120002");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

uint32_t value = 0;
int led = LED_BUILTIN;
uint8_t iLoop = 0;
uint32_t uPressure1NoLoad, uPressure2NoLoad;

#define SERVICE_UUID "8aaeec2c-7f43-11ee-b962-0242ac120002"

// update pressure 50 times per second



//Pins
#define VBATPIN A13
#define VPRESSURE1PIN A2
#define VPRESSURE2PIN A3

void PrintPinVolts(uint uPin, float fVolts){
  Serial.print("Pin:");
  Serial.print(uPin);
  Serial.print(" ");
  Serial.print(fVolts);
  Serial.println(" mVolts");
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

uint32_t getAnalogData(uint uPin) {
  float fMeasuredV = analogReadMilliVolts(uPin);
  if (VERBOSE){PrintPinVolts(uPin,fMeasuredV);};
  return (uint32_t(round(fMeasuredV)));
};

uint32_t getVoltage() {
  uint32_t uVolts = getAnalogData(VBATPIN);
  uVolts *= 2;    // we divided by 2, so multiply backfloat measuredvbat = analogReadMilliVolts(VBATPIN);
  return (uVolts);
};

uint32_t getTemperature() {
  //placeholder for future capabilities to get temperature.
  return (0);
};

void initPressureNoLoads(){
  //get Pressure Voltage at start-up, which we will assume happens at air speed of zero.
  delay(500);
  uPressure1NoLoad = getPressureNoLoad(VPRESSURE1PIN);
  uPressure2NoLoad = getPressureNoLoad(VPRESSURE2PIN);
  Serial.print("P1 NoLoad:");
  Serial.println(uPressure1NoLoad);
  Serial.print("P2 NoLoad:");
  Serial.println(uPressure2NoLoad);
};

uint32_t getPressureNoLoad(uint uPin){
  //  take average over a bit of time
  uint32_t uSum=0;
  int i;
  #define POINTS 5
  for (i = 0; i < POINTS; i++){
    uSum += getAnalogData(uPin);
    if(VERBOSE){Serial.println(uSum);};
    delay(200);
  };
  return (uSum/POINTS);
};



static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      String value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000); // Delay a second between loops.
} // End of loop
