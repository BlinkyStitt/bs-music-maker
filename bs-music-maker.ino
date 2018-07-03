#define DEBUG
#include <bs_debug.h>

// TODO: comment this out for night sounds. uncomment this for motion activated player (wip)
//#define MOTION_ACTIVATED

// based on
// https://github.com/adafruit/Adafruit_VS1053_Library/blob/master/examples/feather_player/feather_player.ino

#include "Arduino.h"
#include <Adafruit_VS1053.h>
#include <EDB.h>
#include <FastLED.h>
#include <IniFile.h>
#include <SD.h>
#include <SPI.h>

// These are the pins used
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define SDCARD_CS 5   // Card chip select pin. already wired for us
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define VBAT_PIN 9    // already wired for us  // A7
#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ 11 // VS1053 Data request, ideally an Interrupt pin
#define LED_DATA 12    // TODO: what pin?
#define RED_LED 13     // already wired for us
#define SPI_MISO 22
#define SPI_MOSI 23
#define SPI_SCK 24

#define LED_CHIPSET NEOPIXEL
const int num_LEDs = 16;
CRGB leds[num_LEDs];

Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, SDCARD_CS);

#define MAX_PLAYLIST_TRACKS 255

typedef struct {
  char filename[12]; // 8.3
} Track;

typedef struct {
  int repeat = 0;
  unsigned int database_id = 0;

  int num_tracks = 0;
  Track tracks[MAX_PLAYLIST_TRACKS];

  int next_track = 0;
  int play_count = 0;
} Playlist;

// rotating "base color" used by some patterns
uint8_t g_hue = 0;

// define database ids
#if defined(MOTION_ACTIVATED)

// TODO: v2: don't hard code this. loop over directories to create playlists instead
#define PLAYLIST_MUSIC 0
#define NUM_PLAYLISTS 1

#else

// TODO: v2: don't hard code this. loop over directories to create playlists instead
#define PLAYLIST_SPOKEN_WORD 0
#define PLAYLIST_NIGHT_SOUNDS 1
#define NUM_PLAYLISTS 2

#endif

Playlist playlists[NUM_PLAYLISTS];

// these are set by config or fallback to defaults
// TODO: making this unsigned makes IniConfig sad. they shouldn't ever be negative though!
int default_brightness, frames_per_second;

void setup() {
  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  // pinMode(8, INPUT_PULLUP);

#ifdef DEBUG
  Serial.begin(115200);

  delay(1000);
  while (!Serial) {
    ; // wait for serial port to connect so we can watch all prints during setup
  }
#endif

  DEBUG_PRINTLN(F("Setting up..."));

  if (!musicPlayer.begin()) { // initialise the music player
    DEBUG_PRINTLN(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1)
      ;
  }

  DEBUG_PRINTLN(F("VS1053 found"));

  if (!SD.begin(SDCARD_CS)) {
    DEBUG_PRINTLN(F("SD failed, or not present"));
    while (1)
      ; // don't do anything more
  }
  delay(100); // SD sometimes takes some time to wake up
  DEBUG_PRINTLN("SD OK!");

  setupConfig();

  setupLights();

  // Set volume for left, right channels. lower numbers == louder volume!
  // TODO: config volume off SD card
  musicPlayer.setVolume(10, 10);

  musicPlayer.sineTest(0x44, 500); // Make a tone for 500ms to indicate VS1053 is working

  loadPlaylists();

  DEBUG_PRINTLN(F("Starting..."));
}

/*
 * this stuff was in its own .ino files, but something was broken about it
 */

// audio-playing and database stuff

// TODO: what table size?
#define TABLE_SIZE 4096 * 2

// The max number of records that should be created = (TABLE_SIZE - sizeof(EDB_Header)) / sizeof(LogEvent).
// If you try to insert more, operations will return EDB_OUT_OF_RANGE for all records outside the usable range.

const char *db_name = "playlist.db";
File db_file;

// Arbitrary record definition for this table.
// This should be modified to reflect your record needs.
typedef struct PlaylistData {
  int playlist_id;
  int next_track;
  int play_count;
};
PlaylistData playlist_data_buffer;

// The read and write handlers for using the SD Library
// Also blinks the led while writing/reading
// database entries start at 1, but that's crazy
inline void writer(unsigned long address, const byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);
  db_file.seek(address + 1);
  db_file.write(data, recsize);
  db_file.flush();
  digitalWrite(RED_LED, LOW);
}

inline void reader(unsigned long address, byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);
  db_file.seek(address + 1);
  db_file.read(data, recsize);
  digitalWrite(RED_LED, LOW);
}

// Create an EDB object with the appropriate write and read handlers
// NOTE! These handlers do NOT open or close the database file!
EDB db(&writer, &reader);

void createTable() {
  DEBUG_PRINT("Creating table... ");
  // create table starting at address 0
  db.create(0, TABLE_SIZE, (unsigned int)sizeof(PlaylistData));
  DEBUG_PRINTLN("DONE");
}

void setupDatabase() {
  // open database if it exists, create database if it doesn't
  if (SD.exists(db_name)) {
    db_file = SD.open(db_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, especially after cold start
    // Let's try one more time
    if (!db_file) {
      db_file = SD.open(db_name, FILE_WRITE);
    }

    if (db_file) {
      DEBUG_PRINT("Opening current table... ");
      EDB_Status result = db.open(0);
      if (result == EDB_OK) {
        DEBUG_PRINTLN("DONE");
      } else {
        DEBUG_PRINTLN("ERROR");
        DEBUG_PRINT("Did not find database in the file ");
        DEBUG_PRINTLN(db_name);

        createTable();
      }
    } else {
      DEBUG_PRINT("Could not open file ");
      DEBUG_PRINTLN(db_name);
      // TODO: loop here since we can't proceed?
      return;
    }
  } else {
    db_file = SD.open(db_name, FILE_WRITE);

    createTable();
  }

  recordLimit();
  countRecords();

  closeDatabase();
}

bool openDatabase() {
  db_file = SD.open(db_name, FILE_WRITE);

  if (db_file) {
    return true;
  } else {
    DEBUG_PRINT("Could not open file ");
    DEBUG_PRINTLN(db_name);
    return false;
  }
}

void closeDatabase() { db_file.close(); }

// TODO: do I need a * or a & or something for playlist?
void loadTracks(char *dir, Playlist playlist) {
  static File root;

  // TODO: make sure the database is open

  // Starting to index the SD card for MP3/AAC.
  DEBUG_PRINT(F("Loading tracks from "));
  DEBUG_PRINTLN(dir);
  root = SD.open(dir);

  while (true) {
    File file = root.openNextFile();
    if (!file) {
      // If no more files, break out.
      break;
    }
    String file_string = file.name(); // put file in string

    DEBUG_PRINT(F("Found: "));
    DEBUG_PRINT(file_string);

    if (!is_audio(file_string)) {
      DEBUG_PRINTLN(F(" (Skipped)"));

      file.close();
      continue;
    }

    // TODO: wth. the .WAV file is getting another name stuck to the end of it
    strcpy(playlist.tracks[playlist.num_tracks].filename, file.name());

    file.close();

    DEBUG_PRINT(F("Saved to track list as: #"));
    DEBUG_PRINT(playlist.num_tracks);
    DEBUG_PRINT(F(" "));
    DEBUG_PRINTLN(playlist.tracks[playlist.num_tracks].filename);

    playlist.num_tracks++;
  }

  bool update_record = false;
  EDB_Status result;

  result = db.readRec(playlist.database_id, EDB_REC playlist_data_buffer);
  if (result == EDB_OUT_OF_RANGE) {
    // this playlist is not in the database. add it now

    // TODO: i think this stores the id in the database twice...
    playlist_data_buffer.playlist_id = playlist.database_id;
    playlist_data_buffer.next_track = 0;
    playlist_data_buffer.play_count = 0;

    update_record = true;
  } else if (playlist_data_buffer.next_track > playlist.num_tracks) {
    // this playlist is in the database, but next_track has an invalid value
    playlist_data_buffer.next_track = 0;

    update_record = true;
  }

  playlist.next_track = playlist_data_buffer.next_track;
  playlist.play_count = playlist_data_buffer.play_count;

  if (update_record) {
    result = db.updateRec(playlist.database_id, EDB_REC playlist_data_buffer);
    // TODO: make sure update worked
  }

  DEBUG_PRINT(F("next_track: "));
  DEBUG_PRINTLN(playlist.next_track);

  DEBUG_PRINT(F("play_count: "));
  DEBUG_PRINTLN(playlist.play_count);

  DEBUG_PRINT(F("num_tracks: "));
  DEBUG_PRINTLN(playlist.num_tracks);
}

void loadPlaylists() {
  openDatabase();

  // TODO: v2: recurse dirs for playlists instead of hard coding
  // TODO: v2: read this from config in /words instead

#ifdef MOTION_ACTIVATED
  playlists[PLAYLIST_MUSIC].database_id = PLAYLIST_MUSIC;
  playlists[PLAYLIST_MUSIC].repeat = 0;
  loadTracks((char *)"/music", playlists[PLAYLIST_MUSIC]);
#else
  playlists[PLAYLIST_SPOKEN_WORD].database_id = PLAYLIST_SPOKEN_WORD;
  playlists[PLAYLIST_SPOKEN_WORD].repeat = 7;
  loadTracks((char *)"/words", playlists[PLAYLIST_SPOKEN_WORD]);

  playlists[PLAYLIST_NIGHT_SOUNDS].database_id = PLAYLIST_NIGHT_SOUNDS;
  playlists[PLAYLIST_NIGHT_SOUNDS].repeat = 3;
  loadTracks((char *)"/sounds", playlists[PLAYLIST_NIGHT_SOUNDS]);
#endif

  closeDatabase();

  // If DREQ is on an interrupt pin we can do background audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
}

// pass play_count and current_track by reference so we can support multiple playlists
void playTrackFromPlaylist(Playlist playlist) {
  if (!musicPlayer.stopped()) {
    // something is already playing. abort
    // we also can't use the database while something is playing
    return;
  }

  const int track_id = playlist.next_track;

  openDatabase();

  // prepare the next track if we have played this one enough times
  playlist.play_count++;
  DEBUG_PRINT(F("Play count: "));
  DEBUG_PRINTLN(playlist.play_count);
  if (playlist.play_count >= playlist.repeat) {
    playlist.play_count = 0;

    playlist.next_track++;
    if (playlist.next_track >= playlist.num_tracks) {
      playlist.next_track = 0;
    }
    DEBUG_PRINT(F("Saving next track to database: "));
    DEBUG_PRINTLN(playlist.next_track);
  }

  // update the database (play_count has definitely changed. next_track may have)
  playlist_data_buffer.playlist_id = playlist.database_id;
  playlist_data_buffer.next_track = playlist.next_track;
  playlist_data_buffer.play_count = playlist.play_count;
  EDB_Status result = db.updateRec(playlist.database_id, EDB_REC playlist_data_buffer);
  // TODO: make sure update worked

  closeDatabase();

  DEBUG_PRINT(F("Playing track: #"));
  DEBUG_PRINT(track_id);
  DEBUG_PRINT(F(" "));
  DEBUG_PRINT(playlist.tracks[track_id].filename);

  // Start playing the file. This sketch continues to run while the file plays.
  musicPlayer.startPlayingFile(playlist.tracks[track_id].filename);

  // A brief delay for the library to read file info
  FastLED.delay(5);

  // TODO: if it isn't playing, return false so we can log and then retry or something
}

/*
 * END things that should be moved into their own ino files
 */

#ifdef MOTION_ACTIVATED

// loop for motion activated sounds for a adopted porta potty
void loop() {
  while (musicPlayer.stopped()) {
    // TODO: wait for PIR sensor, then playTrackFromPlaylist(PLAYLIST_MUSIC)
    playTrackFromPlaylist(playlists[PLAYLIST_MUSIC]);
  }

  updateLights();
  FastLED.delay(1000 / frames_per_second);
}

#else

// loop for night sounds
void loop() {
  playTrackFromPlaylist(playlists[PLAYLIST_SPOKEN_WORD]);
  while (!musicPlayer.stopped()) {
    updateLights();
    FastLED.delay(1000 / frames_per_second);
  }

  while (true) {
    playTrackFromPlaylist(playlists[PLAYLIST_NIGHT_SOUNDS]);

    updateLights();
    // NOTE! this is 100, not 1000 so that there is less delay when it loops
    // TODO: check if this is really necessary
    FastLED.delay(100 / frames_per_second);
  }
}

#endif
