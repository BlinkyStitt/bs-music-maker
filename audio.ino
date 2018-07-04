
bool is_audio(String filename) {
  // TODO: there are more supported formats.
  return filename.endsWith(F(".MP3")) or filename.endsWith(F(".AAC")) or filename.endsWith(F(".MP4")) or
         filename.endsWith(F(".M4A")) or filename.endsWith(F(".WAV"));
}

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