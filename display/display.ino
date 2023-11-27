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
#include <Fonts/FreeSans9pt7b.h>
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

//OLED Constants
//portrait 0|2  landscape 1|3
#define LANDSCAPE 0
#define PORTRAIT 1
///< Battery Icon Width
#define BATTICON_WIDTH 15   
//((BATTICON_WIDTH - 6) / 3) ///< Battery Icon Bar Width
#define BATTICON_BARWIDTH3 3

//NeoPixel Constants
#define LED_COUNT 8
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
struct Val {
  u_int32_t uVal;
  unsigned long uLastMillis;
};

///////////////////////////////
// Data Structures
///////////////////////////////
struct ProbeData {
  // raw data
  // -probe data
  // --connected (bool)
  bool bConnected;
  // --pressureFwd
  struct Val sPressureFwd;
  // --pressure45
  struct Val sPressure45;
  // --pressureFwdT0
  struct Val sPressureFwdT0;
  // --pressure45T0
  struct Val sPressure45T0;
  // --voltage
  struct Val sVolts;
  // --temperature
  struct Val sTemp;
};

struct DisplayData {
  uint32_t uVolts;  //in millivolts
  //temp
  //barometric pressure
  //AQI
};

struct Settings {
  //airspeed units  mph,kts,m/s
  uint8_t iVelUnit;
  //airspeed conversion constant
  //barometric units inHg,mb,hPa
  //altitude units ft,m
  //brightness level [0-4]
  uint8_t iBrightnessLevel;
  //flap position [0-3]
  uint8_t iFlapPos;
  //cruise AoA
  float fAlphaCruise;
  //target AoA
  float fAlphaTarget;
  //stall AoA
  float fAlphaStall;
  //log interval (milliseconds)
  uint16_t uLogInterval;
};

struct Out {
#define NUM_AOA_CHARS 2
#define NUM_VEL_CHARS 3
#define NUM_AQI_CHARS 3
#define NUM_INFO_CHARS 5
  // Pressures Valid and Current
  bool bPressuresValid;
  // Angle of Attack chars(deg)
  char sAlpha[NUM_AOA_CHARS+1];
  // Angle of Attack float(deg)
  float fAlpha;
  // Airspeed chars
  char sVel[NUM_VEL_CHARS+1];
  // Airspeed float
  float fVel;
  // Air Quality
  char sAQI[NUM_AQI_CHARS+1];
  // Barometric pressure
  // Barometric Units
  // Altitude
  // Altitude Units
  // Probe Temp (C)
  // Display Temp (C)
  // Info Lines
  char sInfo1[NUM_INFO_CHARS+1];
  char sInfo2[NUM_INFO_CHARS+1];
  char sInfo3[NUM_INFO_CHARS+1];
  char sInfo4[NUM_INFO_CHARS+1];
  // Battery Voltage
  uint8_t uProbeVolts; //in 0.1 volts
  uint8_t uDisplayVolts; //in 0.1 volts
  // Last Data Millis
  unsigned long uLastDataMillis;
};

////////////////////////////////////////////////////
//  GLOBALS
////////////////////////////////////////////////////
Adafruit_SSD1306 g_display = Adafruit_SSD1306(128, 32, &WIRE);
Adafruit_NeoPixel g_strip(LED_COUNT, NEOPIXELDATAPIN, NEO_GRBW + NEO_KHZ800);
unsigned long gMillis;
float g_afAoABoundary[LED_COUNT];
uint32_t g_auDefaultColors[LED_COUNT];

struct Settings gs;
struct ProbeData gpd;
struct DisplayData gdd;
struct Out go;

// 'Constant' Arrays
uint8_t g_auBrightness[] = {1,3,5,8,25};
char *asVelUnit[] = {"m/s","kts","mph"};

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
//  General Functions
////////////////////////////////////////////////////
void initGlobals(){
  gMillis = millis();
  initSettings();
  memset(&gpd, 0, sizeof(ProbeData));
  memset(&gdd, 0, sizeof(DisplayData));
  memset(&go, 0, sizeof(Out));
  setAoABoundaries();
  g_auDefaultColors[7] = g_strip.Color(RED);
  g_auDefaultColors[6] = g_strip.Color(RED);
  g_auDefaultColors[5] = g_strip.Color(ORANGE);
  g_auDefaultColors[4] = g_strip.Color(BLUE);
  g_auDefaultColors[3] = g_strip.Color(YELLOW);
  g_auDefaultColors[2] = g_strip.Color(YELLOW);
  g_auDefaultColors[1] = g_strip.Color(GREEN);
  g_auDefaultColors[0] = g_strip.Color(GREEN);
}

bool isBlinkOn( unsigned long uCurMillis, u_int16_t uSpeed = NORMAL){
// Function to regulate the speed of a blinking LED
  // Calculate the time elapsed since the last change of state
  unsigned long uElapsedTime = uCurMillis % uSpeed;

  // Determine if the LED should be lit or unlit based on the elapsed time
  return (uElapsedTime < uSpeed / 2);
}

void sprintfIntLen(char *szBuf, int iNum, uint8_t uLen) {
  //function assumes szBuf is zero terminated at uLen+1 already
    // Handle the sign
    if (iNum < 0) {
        *szBuf++ = '-';
        if (uLen == 1) {
            return;
        } else {
            uLen--;
        }
    }
    // Determine the length of the number
    int numLength = snprintf(NULL, 0, "%d", iNum);
    // Check if the length is less than or equal to the specified limit
    if (numLength <= uLen) {
        sprintf(szBuf, "%d", iNum);
    } else {
        // If the length exceeds the limit, print 'e' for each character beyond the limit
        for (uint8_t i = 0; i < uLen; ++i) {
            *szBuf++ = 'e';
        }
    }
}


////////////////////////////////////////////////////
//  Data Functions
////////////////////////////////////////////////////
bool isValid(struct Val *pVal, unsigned long uMaxAge = MAX_CRITICAL_DELAY){
  //used gMillis as a global to save speed (I think)
  if ( (gMillis-pVal->uLastMillis) > uMaxAge ){
    return false;
  }else{
    return true;
  };
};

float voltsToPSI(uint32_t uVolts){
#define A .0419
#define B -0.8279
  return(A*float(uVolts) + B);
}

float voltsToPascals(uint32_t uVolts){
#define PA_PER_PSI 6894.76
  return(voltsToPSI(uVolts) * PA_PER_PSI);
}

float getPfwd(){
  return(voltsToPascals(gpd.sPressureFwd.uVal));
}

bool validatePressures() {
  static uint32_t uPFwdT0 = -1;
  static uint32_t uP45T0 = -1;
  if (!isValid(&gpd.sPressureFwd)) {return(false);};
  if (!isValid(&gpd.sPressure45)) {return(false);};
  if (!isValid(&gpd.sPressureFwdT0)) {return(false);};
  if (!isValid(&gpd.sPressure45T0)) {return(false);};
  //set statics to initial values if still -1
  if (-1 == uPFwdT0) {
    uPFwdT0 = gpd.sPressureFwdT0.uVal;
  }
  if (-1 == uP45T0) {
    uP45T0 = gpd.sPressure45T0.uVal;
  }
  //ensure has not changed
  if (uPFwdT0 != gpd.sPressureFwdT0.uVal){
    return(false);
  }
  if (uP45T0 != gpd.sPressure45T0.uVal){
    return(false);
  }
  return(true);
}

////////////////////////////////////////////////////
//  Analog Data Functions
////////////////////////////////////////////////////
uint32_t getAnalogData(uint uPin) {
  float fMeasuredV = analogReadMilliVolts(uPin);
  // #ifdef VERBOSE
  // PrintPinVolts(uPin,fMeasuredV);
  // #endif
  return (uint32_t(round(fMeasuredV)));
};

uint32_t getVoltage() {
  //get battery voltage in millivolts
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

void getDisplayUnitData(){
  gdd.uVolts = getVoltage();
  //TODO: get other data from display unit environmental module
}

////////////////////////////////////////////////////
//  EEPROM Functions
////////////////////////////////////////////////////
void initSettings(){
  //TODO: get this from EEPROM
  memset(&gs, 0, sizeof(Settings));
  gs.iBrightnessLevel = 3;
  gs.iFlapPos = 0;
  gs.iVelUnit = 1; //m/s,kts,mph
  gs.uLogInterval = 1000; //once per second
  //took initial guess from USA 35b plots
  gs.fAlphaCruise = 3.; //Cl/Cd max
  gs.fAlphaTarget = 12.; //Cl max
  gs.fAlphaStall = 18.; //end of graph 
};

////////////////////////////////////////////////////
//  OLED Display Functions
////////////////////////////////////////////////////
void renderBattery(int16_t x, int16_t y, uint8_t uVolts) {
  // Render the battery icon if requested
  if (255 == uVolts) {return;};  //don't render on 255
  // Draw the base of the battery
  g_display.writeLine( x + 1,  y,
             x + BATTICON_WIDTH - 4,  y,  1);
  g_display.writeLine( x,  y + 1,  x,
             y + 5,  1);
  g_display.writeLine( x + 1,  y + 6,
             x + BATTICON_WIDTH - 4,  y + 6,
             1);
  g_display.writePixel( x + BATTICON_WIDTH - 3,  y + 1,
             1);
  g_display.writePixel( x + BATTICON_WIDTH - 2,  y + 1,
             1);
  g_display.writeLine( x + BATTICON_WIDTH - 1,  y + 2,
             x + BATTICON_WIDTH - 1,  y + 4,
             1);
  g_display.writePixel( x + BATTICON_WIDTH - 2,  y + 5,
             1);
  g_display.writePixel( x + BATTICON_WIDTH - 3,  y + 5,
             1);
  g_display.writePixel( x + BATTICON_WIDTH - 3,  y + 6,
             1);

  // Draw the appropriate number of bars
  if ((uVolts <= 255) && (uVolts >= 41)) {
    // Three bars
    for (uint8_t i = 0; i < 3; i++) {
      g_display.fillRect( x + 2 + (i * BATTICON_BARWIDTH3),
                 y + 2, BATTICON_BARWIDTH3 - 1, 3,  1);
    }
  } else if ((uVolts < 41) && (uVolts >= 38)) {
    // Two bars
    for (uint8_t i = 0; i < 2; i++) {
      g_display.fillRect( x + 2 + (i * BATTICON_BARWIDTH3),
                 y + 2, BATTICON_BARWIDTH3 - 1, 3,  1);
    }
  } else if ((uVolts < 38) && (uVolts >= 34)) {
    // One bar
    g_display.fillRect( x + 2,  y + 2,
              BATTICON_BARWIDTH3 - 1, 3,  1);
  } else {
    // No bars
  }
}


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


void showOLED(){
  uint16_t uFlapBarLen;
  g_display.setRotation(PORTRAIT);
  g_display.setTextSize(1);

  g_display.clearDisplay();

  //Top line AoA (2 digits)
  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 17);
  g_display.print(go.sAlpha);

  //Top Line Deg symbol (3x3px box)
  g_display.writePixel(30,0,1);
  g_display.writePixel(29,0,1);
  g_display.writePixel(28,1,1);
  g_display.writePixel(31,1,1);
  g_display.writePixel(28,2,1);
  g_display.writePixel(31,2,1);
  g_display.writePixel(30,3,1);
  g_display.writePixel(29,3,1);

  //Line 2 Flap Bars
  uFlapBarLen = 30 * gs.iFlapPos;
  g_display.writeFastHLine(0,20,uFlapBarLen,1);
  g_display.writeFastHLine(0,21,uFlapBarLen,1);

  //Line 3 Airspeed (3 digits)
  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 40); //13px + 3px line
  g_display.print(go.sVel);

  //Line 4 Airspeed units (mph|kts|m/s) label
  g_display.setFont(NULL);
  g_display.setCursor(10, 44);  //7px + 1px line
  g_display.print(asVelUnit[gs.iVelUnit]);

  //Line 5 Air quality (3 digits)
  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 67);
  g_display.print(go.sAQI);

  //Line 6 "AQI" Label
  g_display.setFont(NULL);
  g_display.setCursor(10, 71);  //7px + 1px line
  g_display.print("AQI");

  //Line 7 "INFO1"
  g_display.setFont(NULL);
  g_display.setCursor(0, 83);  //7px + 1px line
  g_display.print(go.sInfo1);
  //Line 8 "INFO2"
  g_display.setCursor(0, 92);  //7px + 1px line
  g_display.print(go.sInfo2);
  //Line 9 "INFO3"
  g_display.setCursor(0, 101);  //7px + 1px line
  g_display.print(go.sInfo3);
  //Line 10 "INFO4"
  g_display.setCursor(0, 110);  //7px + 1px line
  g_display.print(go.sInfo4);

  //Bottom battery symbols
  renderBattery(0,120,go.uProbeVolts);
  renderBattery(16,120,go.uDisplayVolts);

  // Display the changes
  g_display.display();
};


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
void initStrip(){
  g_strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  g_strip.show();            // Turn OFF all pixels ASAP
  g_strip.setBrightness(g_auBrightness[gs.iBrightnessLevel]); //set brightness level

  stripTest();
};


void stripColor(uint8_t uR, uint8_t uG, uint8_t uB, uint8_t uAlpha = 0) {
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
}


void stripTest() {
  int i;
  for (i=0;i < LED_COUNT;i++){
      g_strip.setPixelColor(i, g_auDefaultColors[i]);
      }
  g_strip.show();
}


void showStrip(){
  int i;
  //flash CO warning
  //flash connection status
  if (!gpd.bConnected){
      stripColor(OFF);
      if (isBlinkOn(gMillis,SLOW)){
      g_strip.setPixelColor(0, VIOLET);
    } 
    else {
      g_strip.setPixelColor(0, OFF);
    }
    g_strip.show();
    return;
  }
  //show AoA 
  if (go.fAlpha < gs.fAlphaCruise) {
    //if above Alpha Cruise, turn off the AoA strip
    stripColor(OFF);
    return;
  }
  //else show the AoA strip 
  for (i=0;i < LED_COUNT;i++){
    if (go.fAlpha > g_afAoABoundary[i]){
      g_strip.setPixelColor(i, g_strip.Color(OFF));
    } else {
      g_strip.setPixelColor(i, g_auDefaultColors[i]);
    }
  }
  //if above Alpha Stall, flash the top light
  if (go.fAlpha > gs.fAlphaStall){
    if (isBlinkOn(gMillis,FAST)){
      g_strip.setPixelColor(7, RED);
    } 
    else {
      g_strip.setPixelColor(7, OFF);
    }   
  }
  g_strip.show();
};


void setAoABoundaries(){
  float fSlope;
  g_afAoABoundary[0] = gs.fAlphaCruise; //bottom green led
  fSlope = (gs.fAlphaTarget-gs.fAlphaCruise)/4.0;
  g_afAoABoundary[1] = fSlope +gs.fAlphaCruise;
  g_afAoABoundary[2] = (2.0*fSlope)+gs.fAlphaCruise;
  g_afAoABoundary[3] = (3.0*fSlope)+gs.fAlphaCruise;
  g_afAoABoundary[4] = gs.fAlphaTarget; //blue led
  fSlope = (gs.fAlphaStall-gs.fAlphaTarget)/3.0;
  g_afAoABoundary[5] = fSlope +gs.fAlphaTarget;
  g_afAoABoundary[6] = (2.0*fSlope)+gs.fAlphaTarget;
  g_afAoABoundary[7] = gs.fAlphaStall; //top red led
}
////////////////////////////////////////////////////
//  BLE Receiver Functions
////////////////////////////////////////////////////
void initBLE(){
  //TODO: add ble initialization
}

void getProbeData(){
  //TODO: collect probe data from BLE notifications
  static int i = 0;
  static bool bOk = true;
  if (bOk){
    gpd.bConnected = true;
    gpd.sPressureFwd.uVal = ((millis()/5000)%7+20);
    gpd.sPressure45.uVal = ((millis()/500)%10+8);
    gpd.sPressureFwdT0.uVal = 0;
    gpd.sVolts.uVal = ((millis()/1)%3000+2000);
    gpd.sPressureFwd.uLastMillis = gMillis;
    gpd.sPressure45.uLastMillis = gMillis;
    gpd.sPressureFwdT0.uLastMillis = gMillis;
    gpd.sPressure45T0.uLastMillis = gMillis;
    gpd.sVolts.uLastMillis = gMillis;
  } else {
    if (0 == (gMillis/1000) % 7){
      bOk = true;
      i++;
    }
  }
  //throw an error every 32 seconds for 7 seconds
  if (0 == (gMillis/1000) % 32){
    bOk = false;
    if (0 == i) {
      //probe reset error
      gpd.sPressureFwdT0.uVal = 10;
    } else if (1 == i) {
      //data gap error
      gpd.sPressureFwd.uLastMillis = 0;
    } else if (2 == i) {
      gpd.bConnected = false;
    } 
  }
};

////////////////////////////////////////////////////
//  Calculations and Output
////////////////////////////////////////////////////
void calcAlpha() {
  float fRatio;
  fRatio = float(gpd.sPressureFwd.uVal)/float(gpd.sPressure45.uVal);
  #define A 0.0
  #define B -16.908083
  #define C 42.415008
  go.fAlpha = A*fRatio*fRatio + B*fRatio + C;
}

void calcAirspeed() {
  float fVelMS;
  // Constants for standard atmospheric conditions
  // Standard Pressure (Pa)
#define STD_PRESSURE 101325.0  
#define AIR_DENSITY 1.225
  //conversions
#define KTS_PER_M_S 1.94384
#define MPH_PER_M_S 2.23694       

  // Calculate airspeed in m/s using the rearranged Bernoulli's equation
  //will remove std pressure from
  //calculation because our sensor is vented to atomospheric  so 
  //we are already measuring pressure relative to standard.  
  // air density (kg/m^3)
  //fVelMS = sqrt(2 * (getPfwd() - STD_PRESSURE) / AIR_DENSITY);
  fVelMS = sqrt(2 * getPfwd() / AIR_DENSITY);
  //convert to output
  if (1 == gs.iVelUnit) {
    go.fVel = fVelMS*KTS_PER_M_S;
  } else if (2 == gs.iVelUnit) {
    go.fVel = fVelMS*MPH_PER_M_S;
  }
  else {
    go.fVel = fVelMS;
  }
}

void updateOutput(){
  if (validatePressures()) {
    calcAlpha();
    sprintfIntLen(go.sAlpha, int(go.fAlpha+0.5), 2); //write to output characters
    calcAirspeed();
    sprintfIntLen(go.sVel,int(go.fVel+0.5),3); //write to output characters
  } else {
    // Angle of Attack chars(deg)
    sprintf(go.sAlpha,"--");
    // Angle of Attack float(deg)
    go.fAlpha = -999.9;
    // Airspeed chars
    sprintf(go.sVel,"---");
    // Airspeed float
    go.fVel = -999.9;
  }

  // Air Quality
  sprintf(go.sAQI,"---");
  // Barometric pressure
  // Barometric Units
  // Altitude
  // Altitude Units
  // Probe Temp (C)
  // Display Temp (C)

  // Battery Voltage in .1 volts (millivolts/100)
  if ((gpd.bConnected) && (isValid(&gpd.sVolts, MAX_NONCRITICAL_DELAY))) {
     go.uProbeVolts = gpd.sVolts.uVal/100; 
  } else {
    go.uProbeVolts = -1;  //set to 255 so that battery icon not displayed
  }
  go.uDisplayVolts = gdd.uVolts/100;
  // Last Data Millis
  go.uLastDataMillis = gMillis;  
  // Info Linesn(at end for debugging)
  snprintf(go.sInfo1,NUM_INFO_CHARS+1,"%d",gdd.uVolts);
  snprintf(go.sInfo2,NUM_INFO_CHARS+1,"%d",gpd.sPressureFwd.uVal);
  snprintf(go.sInfo3,NUM_INFO_CHARS+1,"%d",gpd.sPressure45.uVal);
  snprintf(go.sInfo4,NUM_INFO_CHARS+1,"%5.f",go.fVel);
  //go.sInfo2[0] = 0;
  //go.sInfo3[0] = 0;
  //go.sInfo4[0] = 0;
};


////////////////////////////////////////////////////
//  MAIN Routine Functions
////////////////////////////////////////////////////
void setup() {
  int i;
  SERIAL_BEGIN(115200);
  SERIAL_PRINTLN("ArduAOA Probe v" VERSION);
  //analogSetPinAttenuation(VBATPIN,ADC_11db);

  initGlobals();

  initDisplay();
  initStrip();
  initEnvironmental();
  initBLE();

  delay(1500);
  g_display.clearDisplay();
  g_display.display();
  stripColor(OFF);
  SERIAL_PRINTLN("Setup complete");
  //debug
  for (i=0; i<LED_COUNT; i++){
    SERIAL_PRINT("i:");
    SERIAL_PRINT(i);
    SERIAL_PRINT(" : ");
    SERIAL_PRINTLN(g_afAoABoundary[i]);
  }
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  gMillis = millis();
  // get BLE data
  getProbeData();

  // get local data
  getDisplayUnitData();

  // do calculations and update output
  updateOutput();

  // update displays
  showOLED();
  showStrip();

  // write data to serial
} // End of loop
