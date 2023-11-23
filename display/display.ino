/*
  Create a BLE client that will receive pressure, temperature and battery voltage notifications from an AoA probe
  and display that data along with data from the local microcontroller on a small OLED screen and a NeoPixel 8x
  LED strip.
  
*/


#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>

//set VERBOSE to 1 if you want more print statements sent to the Serial port.
#define VERBOSE
#define VERSION "0.1"

#ifdef VERBOSE
  #define SERIAL_BEGIN(x) Serial.begin(x)
  #define SERIAL_PRINTLN(x) Serial.println(x)
  #define SERIAL_PRINT(x) Serial.print(x)
#else
  #define SERIAL_BEGIN(x)
  #define SERIAL_PRINTLN(x)
  #define SERIAL_PRINT(x)
#endif

//Pins
#define BUTTONAPIN A8
#define BUTTONBPIN A7
#define BUTTONCPIN A6
#define NEOPIXELDATAPIN A5
#define VBATPIN A13
#define WIRE Wire

//
#define LED_COUNT 8
//portrait 0|2  landscape 1|3
#define LANDSCAPE 0
#define PORTRAIT 1
//max milliseconds for data to age before determined to be invalid
#define MAX_CRITICAL_DELAY 500
#define MAX_NONCRITICAL_DELAY 10000
//types of blink speeds
#define FAST 250
#define NORMAL 1000
#define SLOW 2000
//colors
#define RED 255, 0, 0
#define ORANGE 255, 165, 0
#define YELLOW 255, 255, 0
#define GREEN 0, 255, 0
#define BLUE 0, 0, 255
#define INDIGO 75, 0, 130
#define VIOLET 148, 0, 211
#define WHITE 255,255,255
#define TRUE_WHITE 0, 0, 0, 255
#define OFF   0, 0, 0, 0

//structure to collect data from BLE and keep track of how old it is.
struct Data {
  u_int32_t uVal;
  unsigned long uLastMillis;
};

////////////////////////////////////////////////////
//  GLOBALS
////////////////////////////////////////////////////
Adafruit_SSD1306 g_display = Adafruit_SSD1306(128, 32, &WIRE);
Adafruit_NeoPixel g_strip(LED_COUNT, NEOPIXELDATAPIN, NEO_GRBW + NEO_KHZ800);

uint8_t guBrightness = 50; // Init brightness to 2/5 

////////////////////////////////////////////////////
//  TODO
////////////////////////////////////////////////////
/*
  - add provision for ensuring that probe has not reset after motion has started.  maybe way to check
      is to track zero pressure in EEPROM in the probe and not send if too large from last stored value.
      Also calculating airspeed and showing is good cross check.
  - add provision for adjusting AoA for flaps positions
*/

////////////////////////////////////////////////////
//  General Functions
////////////////////////////////////////////////////
bool isBlinkOn( unsigned long uCurMillis, u_int8_t uSpeed = NORMAL){
// Function to regulate the speed of a blinking LED
  // Calculate the time period for one cycle of the blinking based on the blink speed
  unsigned long uPeriod = 1000 / uSpeed;

  // Calculate the time elapsed since the last change of state
  unsigned long uElapsedTime = uCurMillis % uPeriod;

  // Determine if the LED should be lit or unlit based on the elapsed time
  return (uElapsedTime < uPeriod / 2);
}

////////////////////////////////////////////////////
//  DEBUG Functions
////////////////////////////////////////////////////

void PrintPinVolts(uint uPin, float fVolts){
  SERIAL_PRINT("Pin:");
  SERIAL_PRINT(uPin);
  SERIAL_PRINT(" ");
  SERIAL_PRINT(fVolts);
  SERIAL_PRINTLN(" mVolts");
};


////////////////////////////////////////////////////
//  Data Functions
////////////////////////////////////////////////////
bool isValid(struct Data *pData, unsigned long uCurMillis, unsigned long uMaxAge = MAX_CRITICAL_DELAY){
  if ( (uCurMillis-pData->uLastMillis) > uMaxAge ){
    return false;
  }else{
    return true;
  };
};

String toDisplay(){
  // TODO: function to make data ready for display i.e. limit number of characters
  //  and if too old replace with a dash or other character
};

String toStr(){
  // TODO: function to make data ready for output to csv
};

////////////////////////////////////////////////////
//  Analog Data Functions
////////////////////////////////////////////////////
uint32_t getAnalogData(uint uPin) {
  float fMeasuredV = analogReadMilliVolts(uPin);
  #ifdef VERBOSE
  PrintPinVolts(uPin,fMeasuredV);
  #endif
  return (uint32_t(round(fMeasuredV)));
};

uint32_t getVoltage() {
  uint32_t uVolts = getAnalogData(VBATPIN);
  uVolts *= 2;    // we divided by 2, so multiply backfloat measuredvbat = analogReadMilliVolts(VBATPIN);
  return (uVolts);
};


////////////////////////////////////////////////////
//  Get Environmental Functions
////////////////////////////////////////////////////
uint32_t getTemperature() {
  //TODO: placeholder for future capabilities to get temperature.
  return (0);
};

void initEnvironmental(){
  //TODO: init environmental module
}

////////////////////////////////////////////////////
//  EEPROM Functions
////////////////////////////////////////////////////


////////////////////////////////////////////////////
//  OLED Display Functions
////////////////////////////////////////////////////
void showLogo() {
  g_display.setRotation(LANDSCAPE);

  g_display.clearDisplay();

  // Set text color, size, and location for "ArduAoA" in large font

  g_display.setFont(&FreeSans12pt7b);
  g_display.setTextSize(1);
  g_display.setCursor(30, 20);
  g_display.print(F("ArduAoA"));

  g_display.setRotation(PORTRAIT);

  // Set text color, size, and location for "v1.0" in small font
  g_display.setFont(NULL);
  g_display.setTextSize(1);
  g_display.setCursor(0, 120);
  g_display.print(F("v" VERSION));

  // Display the changes
  g_display.display();
}

void initDisplay(){
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  g_display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  g_display.setTextColor(SSD1306_WHITE);

  //TODO set dimness

  showLogo();

  // Show image buffer on the display hardware.
  g_display.display();
}


////////////////////////////////////////////////////
//  LED Strip Functions
////////////////////////////////////////////////////

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<g_strip.numPixels(); i++) { // For each pixel in strip...
    g_strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    g_strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void initStrip(){
  g_strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  g_strip.show();            // Turn OFF all pixels ASAP
  g_strip.setBrightness(guBrightness);

  colorWipe(g_strip.Color(RED), 50); 
  colorWipe(g_strip.Color(ORANGE), 50); 
  colorWipe(g_strip.Color(YELLOW), 50); 
  colorWipe(g_strip.Color(GREEN), 50); 
  colorWipe(g_strip.Color(BLUE), 50); 
  colorWipe(g_strip.Color(INDIGO), 50); 
  colorWipe(g_strip.Color(VIOLET), 50); 
  colorWipe(g_strip.Color(WHITE), 50); 
  colorWipe(g_strip.Color(TRUE_WHITE), 50); 
  colorWipe(g_strip.Color(OFF), 50); 
}


////////////////////////////////////////////////////
//  BLE Receiver Functions
////////////////////////////////////////////////////
void initBLE(){
  //TODO: add ble initialization
}


////////////////////////////////////////////////////
//  MAIN Routine Functions
////////////////////////////////////////////////////
void setup() {
  SERIAL_BEGIN(115200);
  SERIAL_PRINTLN("ArduAOA Probe v" VERSION);

  initDisplay();
  initEnvironmental();
  initStrip();
  initBLE();

  g_display.clearDisplay();
  g_display.display();
  SERIAL_PRINTLN("Setup complete");
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  // get BLE data

  // get local data

  // do calculations

  // update display

  // write data to serial
} // End of loop
