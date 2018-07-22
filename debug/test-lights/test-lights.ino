#define DEBUG
#define DEBUG_SERIAL_WAIT
#include <bs_debug.h>

#include <FastLED.h>
#include <IniFile.h>
#include <SD.h>
#include <SPI.h>

#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define SDCARD_CS 5 // already wired for us. Card chip select pin. already wired for us
#define VS1053_CS 6 // already wired for us. VS1053 chip select pin (output)
// DREQ should be an Int pin *if possible* (not possible on 32u4)
// pin 9 is also used for checking voltage. i guess we can't do that anymore :(
#define VS1053_DREQ 9 // already wired for us (but not marked on board). VS1053 Data request, ideally an Interrupt pin
#define VS1053_DCS 10 // already wired for us. VS1053 Data/command select pin (output)
#define START_PIN 11  // TODO: what pin?
#define LED_DATA 12   // TODO: what pin?
#define RED_LED 13    // already wired for us
#define SPI_MISO 22
#define SPI_MOSI 23
#define SPI_SCK 24

#define LED_CHIPSET NEOPIXEL
const int num_LEDs = 60;
CRGB leds[num_LEDs];

// TODO: put on SD?
#define LED_FADE_RATE 100

// these are set by config or fallback to defaults
// TODO: making this unsigned makes IniConfig sad. they shouldn't ever be negative though!
int default_brightness, frames_per_second;

void setupLights() {
  DEBUG_PRINT("Setting up lights... ");

  // https://learn.adafruit.com/adafruit-feather-m0-basic-proto/power-management
  // "While you can get 500mA from it, you can't do it continuously from 5V as
  // it will overheat the regulator."
  // TODO: tune this
  FastLED.setMaxPowerInVoltsAndMilliamps(3.3, 300);

  // TODO: something is wrong about this. i'm getting SPI errors
  FastLED.addLeds<LED_CHIPSET, LED_DATA>(leds, num_LEDs).setCorrection(TypicalSMD5050);

  FastLED.setBrightness(default_brightness);
  FastLED.clear();

  // quick setup of lights
  leds[0] = CRGB::Red;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Green;
  leds[3] = CRGB::Blue;
  leds[4] = CRGB::Blue;
  leds[5] = CRGB::Blue;

  FastLED.show();

  DEBUG_PRINTLN("done.");
}

void setup() {
#ifdef DEBUG
#ifdef DEBUG_SERIAL_WAIT
  Serial.begin(115200);

  delay(5000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
#else  // no DEBUG_SERIAL_WAIT
  delay(5000); // todo: tune this. don't get locked out if something crashes
#endif // DEBUG_SERIAL_WAIT
#else  // no DEBUG
  delay(5000); // todo: tune this. don't get locked out if something crashes
#endif // DEBUG

  DEBUG_PRINTLN(F("Setting up..."));

  default_brightness = 100;
  frames_per_second = 50;

  setupLights();

  DEBUG_PRINTLN(F("Starting..."));
}

void loop() {
  static unsigned long last_frame = 0;

  // update the led array every frame
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {
#ifdef DEBUG
    // debugging lights
    int ms_wrapped = millis() % 10000;
    if (ms_wrapped < 1000) {
      DEBUG_PRINT(F(" "));

      if (ms_wrapped < 100) {
        DEBUG_PRINT(F(" "));

        if (ms_wrapped < 10) {
          DEBUG_PRINT(F(" "));
        }
      }
    }
    DEBUG_PRINT(ms_wrapped);

    DEBUG_PRINT(F(": "));
    for (int i = 0; i < num_LEDs; i++) {
      if (leds[i]) {
        // TODO: better logging?
        DEBUG_PRINT(F("X"));
      } else {
        DEBUG_PRINT(F("O"));
      }
    }

    DEBUG_PRINT(F(" | ms since last frame="));
    DEBUG_PRINTLN(millis() - last_frame);

    last_frame = millis();
#endif

    // display the colors
    FastLED.show();
  }

  FastLED.delay(100 / frames_per_second);
}
