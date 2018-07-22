
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

void loadTracks(Playlist &playlist) {
  // TODO: make sure the database is open

  // Starting to index the SD card for MP3/AAC.
  DEBUG_PRINT(F("Loading tracks from "));
  DEBUG_PRINTLN(playlist.directory);

  File root = SD.open(playlist.directory);

  DEBUG_PRINTLN(F("Directory open."));

  EDB_Status result;

  while (playlist.num_tracks < 10) {  // TODO: more than 10 is broken MAX_PLAYLIST_TRACKS) {
    DEBUG_PRINT("Opening file... ");

    File file = root.openNextFile();
    if (!file) {
      DEBUG_PRINTLN("No more files.");
      // If no more files, break out.
      break;
    }

    DEBUG_PRINT("Found ");

    DEBUG_PRINT(file.name());
    DEBUG_PRINT("... ");

    if (!is_audio(file.name())) {
      DEBUG_PRINTLN(F("Skipped."));

      file.close();
      continue;
    }

    DEBUG_PRINTLN("Saving...");

    // TODO: this feels wrong, but it seems to work
    playlist.tracks[playlist.num_tracks] = (Track *)malloc(sizeof(Track));
    strcpy(playlist.tracks[playlist.num_tracks]->filename, file.name());

    file.close();

    DEBUG_PRINT(F("Saved to track list as: #"));
    DEBUG_PRINT(playlist.num_tracks);
    DEBUG_PRINT(" ");

    // TODO: when we print this here it works, but when we print it below it is missing characters
    DEBUG_PRINTLN(playlist.tracks[playlist.num_tracks]->filename);

    playlist.num_tracks++;
  }

  DEBUG_PRINTLN("Closing root...");
  root.close();

  bool update_record = false;

  // TODO: do stuff here with the database
  DEBUG_PRINTLN("Checking database...");
  openDatabase();

  DEBUG_PRINT("Reading database row #");
  DEBUG_PRINTLN(playlist.database_id);

  // TODO: wth. this is showing 0 when the db is first opened, then 1347235411 here
  countRecords();

  // TODO: not sure about this. something in it is broken
  if (false and db.count() >= playlist.database_id) {
    DEBUG_PRINTLN("Existing record.");

    // TODO: this is crashing here because db.count is not being what we expect
    result = db.readRec(playlist.database_id, EDB_REC playlist_data_buffer);

    // TODO: actually check result here

    if (playlist_data_buffer.next_track > playlist.num_tracks) {
      DEBUG_PRINTLN("Existing record has invalid track count.");
      // this playlist is in the database, but next_track has an invalid value
      playlist_data_buffer.next_track = 0;

      update_record = true;
    }
  } else {
    DEBUG_PRINTLN("New record.");

    // TODO: i think this stores the id in the database twice...
    playlist_data_buffer.playlist_id = playlist.database_id;

    playlist_data_buffer.next_track = 0;
    playlist_data_buffer.play_count = 0;

    update_record = true;
  }

  playlist.next_track = playlist_data_buffer.next_track;
  playlist.play_count = playlist_data_buffer.play_count;

  DEBUG_PRINT(F("database_id: "));
  DEBUG_PRINTLN(playlist.database_id);

  DEBUG_PRINT(F("next_track: "));
  DEBUG_PRINTLN(playlist.next_track);

  DEBUG_PRINT(F("play_count: "));
  DEBUG_PRINTLN(playlist.play_count);

  DEBUG_PRINT(F("num_tracks: "));
  DEBUG_PRINTLN(playlist.num_tracks);

  for (int i = 0; i < playlist.num_tracks; i++) {
    DEBUG_PRINT(" - ");
    DEBUG_PRINTLN(playlist.tracks[i]->filename);
  }

  if (update_record) {
    DEBUG_PRINTLN("Updating record. (WIP)");
    // TODO: this is broken
    //result = db.updateRec(playlist.database_id, EDB_REC playlist_data_buffer);
    // TODO: make sure update worked
  }

  closeDatabase();

  if (!playlist.num_tracks) {
    DEBUG_PRINTLN("No tracks loaded!");
    while (1)
      ;
  }

  DEBUG_PRINTLN(F("Tracks loaded."));
}

void loadPlaylists() {
  DEBUG_PRINTLN("Loading playlists...");

  // TODO: v2: recurse dirs for playlists instead of hard coding
  // TODO: v2: read this from config in /words instead

#ifdef MOTION_ACTIVATED
  // music
  playlists[PLAYLIST_MUSIC].database_id = PLAYLIST_MUSIC;
  strcpy(playlists[PLAYLIST_MUSIC].directory, "/music");
  playlists[PLAYLIST_MUSIC].num_tracks = 0;
  playlists[PLAYLIST_MUSIC].next_track = 0;
  playlists[PLAYLIST_MUSIC].play_count = 0;
  playlists[PLAYLIST_MUSIC].repeat = 0;

  loadTracks(playlists[PLAYLIST_MUSIC]);
  // end music
#else
  // spoken word
  playlists[PLAYLIST_SPOKEN_WORD].database_id = PLAYLIST_SPOKEN_WORD;
  strcpy(playlists[PLAYLIST_SPOKEN_WORD].directory, "/words");
  playlists[PLAYLIST_SPOKEN_WORD].num_tracks = 0;
  playlists[PLAYLIST_SPOKEN_WORD].next_track = 0;
  playlists[PLAYLIST_SPOKEN_WORD].play_count = 0;
  playlists[PLAYLIST_SPOKEN_WORD].repeat = 7;

  loadTracks(playlists[PLAYLIST_SPOKEN_WORD]);
  // end spoken word

  // night sounds
  playlists[PLAYLIST_NIGHT_SOUNDS].database_id = PLAYLIST_NIGHT_SOUNDS;
  strcpy(playlists[PLAYLIST_NIGHT_SOUNDS].directory, "/sounds");
  playlists[PLAYLIST_NIGHT_SOUNDS].num_tracks = 0;
  playlists[PLAYLIST_NIGHT_SOUNDS].next_track = 0;
  playlists[PLAYLIST_NIGHT_SOUNDS].play_count = 0;
  playlists[PLAYLIST_NIGHT_SOUNDS].repeat = 2000; // TODO: tune this

  loadTracks(playlists[PLAYLIST_NIGHT_SOUNDS]);
  // end night sounds
#endif

  DEBUG_PRINTLN("Playlists loaded.");
}

// pass play_count and current_track by reference so we can support multiple playlists
void playTrackFromPlaylist(Playlist &playlist) {
  if (!musicPlayer.stopped()) {
    // something is already playing. abort
    // we also can't use the database while something is playing
    return;
  }

  const int track_id = playlist.next_track;

  updateLights(g_lights_on);

  openDatabase();

  updateLights(g_lights_on);

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

  /*
  // TODO: db stuff is broken
  // update the database (play_count has definitely changed. next_track may have)
  playlist_data_buffer.playlist_id = playlist.database_id;
  playlist_data_buffer.next_track = playlist.next_track;
  playlist_data_buffer.play_count = playlist.play_count;
  EDB_Status result = db.updateRec(playlist.database_id, EDB_REC playlist_data_buffer);
  // TODO: make sure update worked
  */

  updateLights(g_lights_on);

  closeDatabase();

  updateLights(g_lights_on);

  char full_path[27];

  strcpy(full_path, playlist.directory);
  strcat(full_path, "/");
  strcat(full_path, playlist.tracks[track_id]->filename);

  DEBUG_PRINT(F("Playing track #"));
  DEBUG_PRINT(track_id);
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(full_path);

  updateLights(g_lights_on);

  // Start playing the file. This sketch continues to run while the file plays.
  musicPlayer.startPlayingFile(full_path);

  updateLights(g_lights_on);

  // A brief delay for the library to read file info
  FastLED.delay(5);

  // TODO: if it isn't playing, return false so we can log and then retry or something
  if (musicPlayer.stopped()) {
    DEBUG_PRINTLN("ERROR PLAYING TRACK!");
  }
}