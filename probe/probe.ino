/*
  Create a BLE server that, once we receive a connection, will send periodic notifications
  of pressure, temperature and battery voltage.
  
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


BLEServer* pServer = NULL;
BLECharacteristic* pChrVoltage = NULL;
BLECharacteristic* pChrTemperature = NULL;
BLECharacteristic* pChrPressureFwd = NULL;
BLECharacteristic* pChrPressure45 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
int led = LED_BUILTIN;
uint8_t iLoop = 0;
uint32_t uTempCounter = 0;  //use temperature as a counter initially until actual data available
uint32_t uPressureFwdNoLoad, uPressure45NoLoad;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

//8aaeec2c-7f43-11ee-b962-0242ac120002  service
//8aaeee98-7f43-11ee-b962-0242ac120002  battery voltage 
//8aaeefba-7f43-11ee-b962-0242ac120002  temp
//8aaef0c8-7f43-11ee-b962-0242ac120002  pressure forward pitot probe
//8aaef244-7f43-11ee-b962-0242ac120002  pressure 45 probe
//8aaef352-7f43-11ee-b962-0242ac120002
//8aaef44c-7f43-11ee-b962-0242ac120002
//8aaef7f8-7f43-11ee-b962-0242ac120002
//8aaef8ca-7f43-11ee-b962-0242ac120002
//8aaef9a6-7f43-11ee-b962-0242ac120002

#define SERVICE_UUID "8aaeec2c-7f43-11ee-b962-0242ac120002"
#define CH_BATTERY_VOLTAGE_UUID "8aaeee98-7f43-11ee-b962-0242ac120002"
#define CH_TEMPERATURE_UUID "8aaeefba-7f43-11ee-b962-0242ac120002"
#define CH_PRESSURE_FWD_UUID "8aaef0c8-7f43-11ee-b962-0242ac120002"
#define CH_PRESSURE_45_UUID "8aaef244-7f43-11ee-b962-0242ac120002"
// update pressure 50 times per second
// bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
#define BLE_DELAY 20
//temperature and voltage once per 2 seconds
#define MOD_TEMP_VOLT 100
//set VERBOSE to 1 if you want more print statements sent to the Serial port.
#define VERBOSE 1
//#define VERSOSE 0

//Pins
#define VBATPIN A13
#define VPRESSUREFWDPIN A2
#define VPRESSURE45PIN A3

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
  return (uTempCounter++);
};

void initPressureNoLoads(){
  //get Pressure Voltage at start-up, which we will assume happens at air speed of zero.
  delay(500);
  uPressureFwdNoLoad = getPressureNoLoad(VPRESSUREFWDPIN);
  uPressure45NoLoad = getPressureNoLoad(VPRESSURE45PIN);
  Serial.print("P1 NoLoad:");
  Serial.println(uPressureFwdNoLoad);
  Serial.print("P2 NoLoad:");
  Serial.println(uPressure45NoLoad);
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



void setup() {
  Serial.begin(115200);
  Serial.println("Starting ArduAOA Probe...");
  iLoop = 0;

  //set input pins
  pinMode(VBATPIN, INPUT);
  pinMode(VPRESSUREFWDPIN, INPUT);
  pinMode(VPRESSURE45PIN, INPUT);

  // set LED to be an output pin
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);

  // initialize the pressure NOLOAD values
  initPressureNoLoads();

  // Create the BLE Device
  BLEDevice::init("ArduAOA Probe");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pChrVoltage = pService->createCharacteristic(
                      CH_BATTERY_VOLTAGE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pChrTemperature = pService->createCharacteristic(
                      CH_TEMPERATURE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pChrPressureFwd = pService->createCharacteristic(
                      CH_PRESSURE_FWD_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pChrPressure45 = pService->createCharacteristic(
                      CH_PRESSURE_45_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );

  // Create BLE Descriptors
  pChrVoltage->addDescriptor(new BLE2902());
  pChrTemperature->addDescriptor(new BLE2902());
  pChrPressureFwd->addDescriptor(new BLE2902());
  pChrPressure45->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting ArduAOA Display connection to notify...");
  digitalWrite(led, LOW);
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        // collect and send pressures
        value = getAnalogData(VPRESSUREFWDPIN);
        value -= uPressureFwdNoLoad;
        pChrPressureFwd->setValue((uint8_t*)&value, 4);
        pChrPressureFwd->notify();

        value = getAnalogData(VPRESSURE45PIN);
        value -= uPressure45NoLoad;
        pChrPressure45->setValue((uint8_t*)&value, 4);
        pChrPressure45->notify();

        // if ready, collect temperature and voltage
        if (0 == iLoop % MOD_TEMP_VOLT) {
          if(VERBOSE){Serial.println("Sending Battery Voltage and Temperature");};
          value = getVoltage();
          pChrVoltage->setValue((uint8_t*)&value, 4);
          pChrVoltage->notify();
          value = getTemperature();
          pChrTemperature->setValue((uint8_t*)&value, 4);
          pChrTemperature->notify();
        }

        delay(BLE_DELAY);

        //flash LED when connected
        if (0 == iLoop % 22){digitalWrite(led, HIGH);}
        if (0 == iLoop % 25){digitalWrite(led, LOW);}
    }
    else {
      //flash LED when not connected, will appear dimly lit
      if (0 == iLoop % 240){digitalWrite(led, HIGH);}
      if (0 == iLoop % 250){digitalWrite(led, LOW);}
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Disconnected. Start advertising...");
        oldDeviceConnected = deviceConnected;
        digitalWrite(led, LOW);
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
        value = 0;
        digitalWrite(led, HIGH);
        Serial.println("Connected. Start sending data...");
    }
    iLoop++;
}