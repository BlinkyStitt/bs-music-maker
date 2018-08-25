// help from
// https://github.com/adafruit/Adafruit_VS1053_Library/blob/master/examples/feather_player/feather_player.ino

#define DEBUG
#define DEBUG_SERIAL_WAIT
#include <bs_debug.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))

#define MOTION_ACTIVATED

#include <Arduino.h>
#include <Adafruit_VS1053.h>
#include <EDB.h>
#include <FastLED.h>
#include <IniFile.h>
#include <RTCZero.h>
#include <SD.h>
#include <SPI.h>

#include "types.h"

// These are the pins used
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define SDCARD_CS 5 // already wired for us. Card chip select pin. already wired for us
#define VS1053_CS 6 // already wired for us. VS1053 chip select pin (output)
// DREQ should be an Int pin *if possible* (not possible on 32u4)
// pin 9 is also used for checking voltage. i guess we can't do that anymore :(
#define VS1053_DREQ 9 // already wired for us (but not marked on board). VS1053 Data request, ideally an Interrupt pin
#define VS1053_DCS 10 // already wired for us. VS1053 Data/command select pin (output)
#define START_PIN 11
#define LED_DATA 12
#define RED_LED 13 // already wired for us
#define SPI_MISO 22
#define SPI_MOSI 23
#define SPI_SCK 24

#define LED_CHIPSET NEOPIXEL

const int num_LEDs = 60;
CRGB leds[num_LEDs];

Adafruit_VS1053_FilePlayer music_player =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, SDCARD_CS);

RTCZero rtc;

// TODO: increase this. dynamically allocate this?
#define MAX_PLAYLIST_TRACKS 10

unsigned int g_num_tracks = 0;
char g_tracks[MAX_PLAYLIST_TRACKS][13] = {{0}};  // 8.3\0
unsigned int g_next_track = 0;
File g_file;
bool g_lights_on = false;
bool g_music_stopped = true;

bool sd_setup = false, config_setup = false;

// these are set by config or fallback to defaults
int default_brightness, frames_per_second, alarm_hours, alarm_minutes, alarm_seconds;

void setup() {
  debug_serial(115200, 2000);

  DEBUG_PRINTLN(F("Setting up..."));

  // for night sounds, this is a button
  // for motion activated, this is a pir sensor
  // NOTE: pir sensor is NOT accurate until it has been plugged in for 20 seconds or so.
  // NOTE: some pir sensors are sensitive to interference from the speaker amp!
  pinMode(START_PIN, INPUT);

  if (!music_player.begin()) { // initialise the music player
    DEBUG_PRINTLN(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1)
      ;
  }

  DEBUG_PRINTLN(F("VS1053 found"));

  while (!SD.begin(SDCARD_CS)) {
    DEBUG_PRINTLN(F("SD failed, or not present. Retrying in 1 second..."));
    delay(1000);
  }
  delay(150); // SD sometimes takes some time to wake up
  DEBUG_PRINTLN("SD OK!");
  sd_setup = true;

  setupConfig();

  //setupLights();

  music_player.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int

  // Set volume for left, right channels. lower numbers == louder volume!
  music_player.setVolume(0, 0);

  music_player.sineTest(0x44, 300); // Make a tone for 500ms to indicate VS1053 is working

  delay(300);

  loadTracks();

  setupInterrupts();

  DEBUG_PRINTLN(F("Starting..."));
}

// run when START_PIN is RISING
void playMotionActivated() {
  playTrack();
}

// loop for motion activated sounds for a adopted porta potty
void loop() {
  // count how many frames we've been turning the lights off
  static int off_frames = 0;

  // give the lights enough frames to fade to black before sleeping
  static const int sleep_frames = 10 * 1000 / (1000 / frames_per_second);

  static const int loop_delay = 100 / frames_per_second;

  updateLights();

  // checking music player is slow so only do it every second. this means there might be a small gap in playback, but lights will stay on
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {
    g_music_stopped = music_player.stopped();
  }

  // TODO: if last motion was 2 minutes ago, stop the music player

  if (g_music_stopped) {
    // TODO: this is simply flapping on and off... test-motion-sensor doesn't have this behavior. wtf is going on!
    // apparently our PIR is sensitive to interference. often this is radio, but our amp causing the same issues.
    // AM312 PIR is less sensitive
    // TODO: it appears to be getting stuck playing music...
    if (digitalRead(START_PIN) == HIGH) {
      // there is still motion. start a new song and stay on
      DEBUG_PRINTLN(F("Motion detected!"));
      playMotionActivated();
      g_lights_on = true;
      off_frames = 0;
    } else {
      // music has ended had there is no motion. turn off
      g_lights_on = false;
      off_frames++;

      if (off_frames >= sleep_frames) {
        DEBUG_PRINTLN(F("Sleeping... (Serial will disconnect)"));

        /*
        // quick and dirty sleep code
        while (digitalRead(START_PIN) == LOW) {
          delay(250);
        }
        */
        // TODO: turn this back on once the rest of the code is more solid. since this kills usb serial which is helpful for debugging
        sleep();  // START_PIN going HIGH will wake us up

        playMotionActivated();
        g_music_stopped = music_player.stopped();
        g_lights_on = true;
        off_frames = 0;
      }
    }
  } else {
    g_lights_on = true;
    off_frames = 0;
  }

  FastLED.delay(loop_delay);
}
