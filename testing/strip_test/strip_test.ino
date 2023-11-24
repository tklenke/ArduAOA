/*
  Create a BLE client that will receive pressure, temperature and battery voltage notifications from an AoA probe
  and display that data along with data from the local microcontroller on a small OLED screen and a NeoPixel 8x
  LED strip.
  
*/


#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>

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
#define ORANGE 255, 80, 0
#define YELLOW 255, 255, 0
#define GREEN 0, 255, 0
#define BLUE 0, 0, 255
#define INDIGO 75, 0, 130
#define VIOLET 148, 0, 211
#define WHITE 255,255,255
#define TRUE_WHITE 0, 0, 0, 255
#define OFF   0, 0, 0, 0
//brightness
uint8_t auBrightness[] = {1,3,5,8,25};

//structure to collect data from BLE and keep track of how old it is.
struct Data {
  u_int32_t uVal;
  unsigned long uLastMillis;
};

////////////////////////////////////////////////////
//  GLOBALS
////////////////////////////////////////////////////
Adafruit_NeoPixel g_strip(LED_COUNT, NEOPIXELDATAPIN, NEO_GRBW + NEO_KHZ800);

uint8_t guBrightness = 8; // Init brightness to 2/5 

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
bool isBlinkOn( unsigned long uCurMillis, u_int16_t uSpeed = NORMAL){
// Function to regulate the speed of a blinking LED
  // Calculate the time elapsed since the last change of state
  unsigned long uElapsedTime = uCurMillis % uSpeed;

  // Determine if the LED should be lit or unlit based on the elapsed time
  return (uElapsedTime < uSpeed / 2);
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
};

void stripAoAtest(){
  g_strip.setPixelColor(7, g_strip.Color(RED));
  g_strip.setPixelColor(6, g_strip.Color(RED));
  g_strip.setPixelColor(5, g_strip.Color(ORANGE));
  g_strip.setPixelColor(4, g_strip.Color(BLUE));
  g_strip.setPixelColor(3, g_strip.Color(YELLOW));
  g_strip.setPixelColor(2, g_strip.Color(YELLOW));
  g_strip.setPixelColor(1, g_strip.Color(GREEN));
  g_strip.setPixelColor(0, g_strip.Color(GREEN));

  g_strip.show();  
};

void stripOff(){
  g_strip.setPixelColor(7, g_strip.Color(OFF));
  g_strip.setPixelColor(6, g_strip.Color(OFF));
  g_strip.setPixelColor(5, g_strip.Color(OFF));
  g_strip.setPixelColor(4, g_strip.Color(OFF));
  g_strip.setPixelColor(3, g_strip.Color(OFF));
  g_strip.setPixelColor(2, g_strip.Color(OFF));
  g_strip.setPixelColor(1, g_strip.Color(OFF));
  g_strip.setPixelColor(0, g_strip.Color(OFF));

  g_strip.show();
}

void stripColor(uint8_t uR, uint8_t uG, uint8_t uB) {
  uint32_t uColor = g_strip.Color(uR,uG,uB);
  g_strip.setPixelColor(7, uColor);
  g_strip.setPixelColor(6, uColor);
  g_strip.setPixelColor(5, uColor);
  g_strip.setPixelColor(4, uColor);
  g_strip.setPixelColor(3, uColor);
  g_strip.setPixelColor(2, uColor);
  g_strip.setPixelColor(1, uColor);
  g_strip.setPixelColor(0, uColor);

  g_strip.show();
  delay(10);
}

////////////////////////////////////////////////////
//  MAIN Routine Functions
////////////////////////////////////////////////////
void setup() {
  int i, j, k;
  SERIAL_BEGIN(115200);
  SERIAL_PRINTLN("ArduAOA Probe v" VERSION);


  initStrip();
  stripAoAtest();

  //cycle brightness
  for (i = 0; i < 256; i++){
    g_strip.setBrightness(i);
    stripAoAtest();
    delay(3);
  }
  //above approx 70 for brightness some of the leds (blue)
  //no longer fully lit, and above 100 goes dark completely
  for (i = 0; i < 90; i++){
    g_strip.setBrightness(i);
    stripAoAtest();
    delay(10);
  }
  for (i = 0; i < 25; i++){
    g_strip.setBrightness(i);
    stripAoAtest();
    delay(10);
  }
  //cycle brightness levels.  {1,3,5,8,25} appears to be good range
  for (i = 0; i < 5; i++) {
    g_strip.setBrightness(auBrightness[i]);
    stripAoAtest();
    delay(100);
  }

  //cycle colors (sort of)
  // g_strip.setBrightness(8);
  // for (i = 0; i < 256; i=i+25){
  //   for (j = 0; j < 256; j=j+25){
  //     for (k = 0; k < 256; k=k+25){
  //       stripColor(i,j,k);
  //     }
  //   }
  // }

  stripAoAtest();

  SERIAL_PRINTLN("Setup complete");
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  unsigned long uMillis;

  uMillis = millis();

  if (isBlinkOn(uMillis, FAST)){
    g_strip.setPixelColor(7, RED);
  } 
  else {
    g_strip.setPixelColor(7, OFF);
  }

  if (isBlinkOn(uMillis,NORMAL)){
    g_strip.setPixelColor(4, BLUE);
  } 
  else {
    g_strip.setPixelColor(4, OFF);
  }

  if (isBlinkOn(uMillis,SLOW)){
    g_strip.setPixelColor(0, GREEN);
  } 
  else {
    g_strip.setPixelColor(0, OFF);
  }

  g_strip.show();

} // End of loop
