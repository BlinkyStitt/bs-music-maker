
inline bool is_hidden(const char *filename) {
  return filename[0] == '_';
}

inline bool is_audio(const char *filename) {
  if (is_hidden(filename)) {
    return false;
  }

  const int filename_length = strlen(filename);

  if (filename_length < 5) {
    return false;
  }

  if (filename[filename_length - 4] != '.') {
    return false;
  }

  char filename_suffix[3];
  strcpy(filename_suffix, &filename[filename_length - 3]);

  // TODO: there are more supported formats.
  if (strcmp(filename_suffix, "MP3") == 0) {
    return true;
  }
  if (strcmp(filename_suffix, "AAC") == 0) {
    return true;
  }
  if (strcmp(filename_suffix, "MP4") == 0) {
    return true;
  }
  if (strcmp(filename_suffix, "M4A") == 0) {
    return true;
  }
  if (strcmp(filename_suffix, "WAV") == 0) {
    return true;
  }

  return false;
}

void loadTracks() {
  // TODO: make sure the database is open

  // Starting to index the SD card for MP3/AAC.
  DEBUG_PRINT(F("Loading tracks from /"));

  File root = SD.open("/");

  DEBUG_PRINTLN(F("Directory open."));

  // TODO: i guess we are using too much RAM. we need to do this more efficiently. we don't need every track in memory
  while (g_num_tracks < MAX_PLAYLIST_TRACKS) {  // TODO: more than 10 is broken MAX_PLAYLIST_TRACKS) {
    DEBUG_PRINT(F("Opening file... "));

    File file = root.openNextFile();
    if (!file) {
      DEBUG_PRINTLN(F("No more files."));
      // If no more files, break out.
      break;
    }

    DEBUG_PRINT(F("Found "));

    DEBUG_PRINT(file.name());
    DEBUG_PRINT(F("... "));

    if (!is_audio(file.name())) {
      DEBUG_PRINTLN(F("Skipped."));

      file.close();
      continue;
    }

    DEBUG_PRINTLN(F("Saving..."));

    strcpy(g_tracks[g_num_tracks], file.name());

    file.close();

    DEBUG_PRINT(F("Saved to track list as: #"));
    DEBUG_PRINT(g_num_tracks);
    DEBUG_PRINT(F(" "));

    // TODO: when we print this here it works, but when we print it below it is missing characters
    DEBUG_PRINTLN(g_tracks[g_num_tracks]);

    g_num_tracks++;
  }

  DEBUG_PRINTLN(F("Closing root..."));
  root.close();

  if (!g_num_tracks) {
    DEBUG_PRINTLN(F("No tracks loaded!"));
    while (1)
      ;
  }

  g_next_track = 0;

  // TODO: this prints properly during setup, but once we enter the loop it is broken.
  printTracks();

  DEBUG_PRINTLN(F("Tracks loaded."));
}

void printTracks() {
  DEBUG_PRINT(F("num_tracks: "));
  DEBUG_PRINTLN(g_num_tracks);

  for (int i = 0; i < g_num_tracks; i++) {
    DEBUG_PRINT(F(" - "));
    DEBUG_PRINTLN(g_tracks[i]);
  }
}

// pass play_count and current_track by reference so we can support multiple playlists
void playTrack() {
  if (!music_player.stopped()) {
    // something is already playing. abort
    // we also can't use the database while something is playing
    return;
  }

  const int track_id = g_next_track;

  DEBUG_PRINT(F("Playing track #"));
  DEBUG_PRINTLN(track_id);

  updateLights();

  // prepare the next track if we have played this one enough times
  g_next_track++;

  if (g_next_track >= g_num_tracks) {
      g_next_track = 0;
  }

  DEBUG_PRINT(F("Next track: "));
  DEBUG_PRINTLN(g_next_track);

  DEBUG_PRINT(F("Playing track #"));
  DEBUG_PRINT(track_id);
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(g_tracks[track_id]);

  updateLights();

  // Start playing the file. This sketch continues to run while the file plays.
  music_player.startPlayingFile(g_tracks[track_id]);

  updateLights();

  // A brief delay for the library to read file info
  FastLED.delay(5);

  // TODO: if it isn't playing, return false so we can log and then retry or something
  if (music_player.stopped()) {
    DEBUG_PRINTLN("ERROR PLAYING TRACK!");
  }
}