//
// Testing for format of OLED display
//

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Default Font: Height (7px) 
// FreeSans9pt7b: Height (13px)
// FreeSans12pt7b: Height (17px)
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>

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

//portrait 0|2  landscape 1|3
#define LANDSCAPE 0
#define PORTRAIT 1
///< Battery Icon Width
#define BATTICON_WIDTH 15   
//((BATTICON_WIDTH - 6) / 3) ///< Battery Icon Bar Width
#define BATTICON_BARWIDTH3 3


////////////////////////////////////////////////////
//  GLOBALS
////////////////////////////////////////////////////
Adafruit_SSD1306 g_display = Adafruit_SSD1306(128, 32, &WIRE);

uint8_t guBrightness = 50; // Init brightness to 2/5 

////////////////////////////////////////////////////
//  OLED Display Functions
////////////////////////////////////////////////////
void renderBattery(int16_t x, int16_t y, uint8_t uVolts) {
  // Render the battery icon if requested
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
  if ((uVolts <= 255) && (uVolts >= 191)) {
    // Three bars
    for (uint8_t i = 0; i < 3; i++) {
      g_display.fillRect( x + 2 + (i * BATTICON_BARWIDTH3),
                 y + 2, BATTICON_BARWIDTH3 - 1, 3,  1);
    }
  } else if ((uVolts < 191) && (uVolts >= 127)) {
    // Two bars
    for (uint8_t i = 0; i < 2; i++) {
      g_display.fillRect( x + 2 + (i * BATTICON_BARWIDTH3),
                 y + 2, BATTICON_BARWIDTH3 - 1, 3,  1);
    }
  } else if ((uVolts < 127) && (uVolts >= 63)) {
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


void showFontTest() {
  g_display.setRotation(PORTRAIT);
  g_display.setTextSize(1);

  g_display.clearDisplay();

  g_display.writePixel(0,0,1);
  g_display.writePixel(31,0,1);
  g_display.writePixel(0,127,1);
  g_display.writePixel(31,127,1);

  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 17);
  g_display.print("18");

  g_display.setFont(&FreeSansBold12pt7b);
  g_display.setCursor(0, 40);
  g_display.print("00");

  g_display.setFont(&FreeMono12pt7b);
  g_display.setCursor(0, 60);
  g_display.print("00");

  g_display.setFont(&FreeMonoBold12pt7b);
  g_display.setCursor(0, 80);
  g_display.print("00");

  g_display.setFont(&FreeSerif12pt7b);
  g_display.setCursor(0, 100);
  g_display.print("15");

  g_display.setFont(&FreeSerifBold12pt7b);
  g_display.setCursor(0, 120);
  g_display.print("00");

  // Display the changes
  g_display.display();
}

void showFontSpacingTest(){
  g_display.setRotation(PORTRAIT);
  g_display.setTextSize(1);

  g_display.clearDisplay();

  g_display.writePixel(0,0,1);
  g_display.writePixel(31,0,1);
  g_display.writePixel(0,127,1);
  g_display.writePixel(31,127,1);

  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 17);
  g_display.print("18");

  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 34);
  g_display.print("18");

  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 47);
  g_display.print("180");

  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 60);
  g_display.print("180");

  //two pixels spacing
  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 79);
  g_display.print("18");

  //three pixels spacing
  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 99);
  g_display.print("18");

  // Set text color, size, and location for "v1.0" in small font
  g_display.setFont(NULL);
  g_display.setCursor(0, 113);
  g_display.print("WWWWW");

  g_display.setCursor(0, 120);
  g_display.print("MMMMM");

  // Display the changes
  g_display.display();
};

void showLayoutTest(){
  g_display.setRotation(PORTRAIT);
  g_display.setTextSize(1);

  g_display.clearDisplay();

  //Top line AoA (2 digits)
  g_display.setFont(&FreeSans12pt7b);
  g_display.setCursor(0, 17);
  g_display.print("18");

  //Top Line Deg symbol (3x3px box)
  // g_display.writePixel(31,0,1);
  // g_display.writePixel(30,0,1);
  // g_display.writePixel(29,0,1);
  // g_display.writePixel(31,1,1);
  // g_display.writePixel(29,1,1);
  // g_display.writePixel(31,2,1);
  // g_display.writePixel(30,2,1);
  // g_display.writePixel(29,2,1);

  g_display.writePixel(30,0,1);
  g_display.writePixel(29,0,1);
  g_display.writePixel(28,1,1);
  g_display.writePixel(31,1,1);
  g_display.writePixel(28,2,1);
  g_display.writePixel(31,2,1);
  g_display.writePixel(30,3,1);
  g_display.writePixel(29,3,1);

  //Line 2 Flap Bars
  g_display.writeFastHLine(0,20,30,1);
  g_display.writeFastHLine(0,21,30,1);

  //Line 3 Airspeed (3 digits)
  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 40); //13px + 3px line
  g_display.print("188");

  //Line 4 Airspeed units (mph|kts|m/s)
  g_display.setFont(NULL);
  g_display.setCursor(10, 44);  //7px + 1px line
  g_display.print("ktp");

  //Line 5 Air quality (3 digits)
  g_display.setFont(&FreeSans9pt7b);
  g_display.setCursor(0, 67);
  g_display.print("100");

  //Line 6 "AQI"
  g_display.setFont(NULL);
  g_display.setCursor(10, 71);  //7px + 1px line
  g_display.print("AQI");

  //Line 7 "INFO1"
  g_display.setFont(NULL);
  g_display.setCursor(0, 83);  //7px + 1px line
  g_display.print("INFO1");
  //Line 8 "INFO2"
  g_display.setCursor(0, 92);  //7px + 1px line
  g_display.print("INFO2");
  //Line 9 "INFO3"
  g_display.setCursor(0, 101);  //7px + 1px line
  g_display.print("INFO3");
  //Line 10 "INFO4"
  g_display.setCursor(0, 110);  //7px + 1px line
  g_display.print("INFO4");
  //Line 11 "INFO5"
  // g_display.setCursor(0, 116);  //7px + 1px line
  // g_display.print("INFO5");
 

  //Bottom battery symbols
  renderBattery(0,120,255);
  renderBattery(16,120,255);

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


void setup()
{
  SERIAL_BEGIN(115200);
  SERIAL_PRINTLN("ArduAOA Probe v" VERSION);

  initDisplay();
  
  delay(1000);
  g_display.clearDisplay();
  g_display.display();
  SERIAL_PRINTLN("Setup complete");
  
}

void loop()
{
  //
  showFontTest();

  delay(2000);

  showFontSpacingTest();

  delay(2000);

  showLayoutTest();

  delay(6000);
}
