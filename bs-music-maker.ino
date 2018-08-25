// help from
// https://github.com/adafruit/Adafruit_VS1053_Library/blob/master/examples/feather_player/feather_player.ino

#define DEBUG
#define DEBUG_SERIAL_WAIT
#include <bs_debug.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))

// TODO: pick one of these
#define MOTION_ACTIVATED
//#define NIGHT_SOUNDS

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

// quick and dirty way to define database ids
#ifdef MOTION_ACTIVATED

#define PLAYLIST_MUSIC 1

#define NUM_PLAYLISTS 1

#endif // MOTION_ACTIVATED

#ifdef NIGHT_SOUNDS

#define PLAYLIST_SPOKEN_WORD 1
#define PLAYLIST_NIGHT_SOUNDS 2

#define NUM_PLAYLISTS 2

#endif // NIGHT_SOUNDS

// TODO: initialize this?
PlaylistData playlist_data_buffer;

// TODO: i think this is wrong. i think i have to init this
Playlist playlists[NUM_PLAYLISTS];

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

  setupConfig();

  setupLights();

  // Set volume for left, right channels. lower numbers == louder volume!
  music_player.setVolume(0, 0);

  music_player.sineTest(0x44, 500); // Make a tone for 500ms to indicate VS1053 is working

  music_player.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int

  //setupDatabase();

  loadPlaylists();

  setupInterrupts();

  DEBUG_PRINTLN(F("Starting..."));
}

/*
 * this stuff was in its own .ino files, but something was broken about it
 * i think i fixed it by moving the struct definitions to types.h, but I'm not sure how best to move my_file and db defs
 */

#define TABLE_SIZE 4096

// The max number of records that should be created = (TABLE_SIZE - sizeof(EDB_Header)) / sizeof(LogEvent).
// If you try to insert more, operations will return EDB_OUT_OF_RANGE for all records outside the usable range.

const char *db_name = "playlist.db";
File my_file;

// The read and write handlers for using the SD Library
// Also blinks the led while writing/reading
// database entries start at 1, but that's crazy
inline void writer(unsigned long address, const byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);

  DEBUG_PRINT("Writing to database ");
  DEBUG_PRINTLN(address);

  my_file.seek(address);
  my_file.write(data, recsize);
  my_file.flush();

  digitalWrite(RED_LED, LOW);
}

inline void reader(unsigned long address, byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);

  DEBUG_PRINT("Reading from database ");
  DEBUG_PRINTLN(address);

  my_file.seek(address);
  my_file.read(data, recsize);

  digitalWrite(RED_LED, LOW);
}

// Create an EDB object with the appropriate write and read handlers
// NOTE! These handlers do NOT open or close the database file!
EDB db(&writer, &reader);

/*
 * END things that should be moved into their own ino files
 */

bool g_lights_on = false;
bool g_music_stopped = true;

#ifdef MOTION_ACTIVATED

// run when START_PIN is RISING
void playMotionActivated() {
  //playTrackFromPlaylist(&playlists[PLAYLIST_MUSIC]);
}

// loop for motion activated sounds for a adopted porta potty
void loop() {
  // count how many frames we've been turning the lights off
  static int off_frames = 0;

  // give the lights enough frames to fade to black before sleeping
  static const int sleep_frames = 10 * 1000 / (1000 / frames_per_second);

  static const int loop_delay = 100 / frames_per_second;

  updateLights(g_lights_on);

  // checking music player is slow so only do it every second. this means there might be a small gap in playback, but lights will stay on
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {
    g_music_stopped = music_player.stopped();
  }

  EVERY_N_MILLISECONDS(1000) {
    printPlaylist(&playlists[PLAYLIST_MUSIC]);
  }

  if (g_music_stopped) {
    // TODO: this is simply flapping on and off... test-motion-sensor doesn't have this behavior. wtf is going on!
    // apparently our PIR is sensitive to interference. often this is radio, but our amp causing the same issues.
    // AM312 PIR is less sensitive
    if (true or digitalRead(START_PIN) == HIGH) {
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
        sleep();

        playMotionActivated();
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

#endif

#ifdef NIGHT_SOUNDS

// volatile because an interrupt can change it
volatile bool g_music_on = true;

// loop for night sounds
void loop() {
  // notice that this is 100 and not 1000. this keeps us from having an audible gap
  static const int loop_delay = 100 / frames_per_second;

  // sleep to save power. waiting for button press (START_PIN to be RISING)
  sleep();

  // button was pressed! start the show
  g_lights_on = true;
  g_music_on = true;
  playTrackFromPlaylist(&playlists[PLAYLIST_SPOKEN_WORD]);

  // wait for the meditation track to finish
  while (!music_player.stopped()) {
     updateLights(g_lights_on);
     FastLED.delay(loop_delay);
  }

  // optionally set timer for night sounds to turn off now that the meditation is over
  if (alarm_hours or alarm_minutes or alarm_seconds) {
    rtc.setTime(0, 0, 0);   // we don't need a real time; we just use this as a timer
    rtc.setAlarmTime(alarm_hours, alarm_minutes, alarm_seconds);
    rtc.enableAlarm(rtc.MATCH_HHMMSS);
  }

  // loop night sounds
  while (true) {
    // the rtc will disable g_music_on if alarms are configured.
    // otherwise night sounds will play until button is pressed // TODO: use bounce library here?
    if (!g_music_on or digitalRead(START_PIN) == HIGH) {
      music_player.stopPlaying();
      break;
    }

    playTrackFromPlaylist(&playlists[PLAYLIST_NIGHT_SOUNDS]);

    updateLights(g_lights_on);
    FastLED.delay(loop_delay);
  }
}

#endif // NIGHT_SOUNDS
