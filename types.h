// Workaround from http://playground.arduino.cc/Code/Enum

#define MAX_PLAYLIST_TRACKS 255

enum BatteryStatus : byte { BATTERY_DEAD, BATTERY_LOW, BATTERY_OK, BATTERY_FULL };

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

// Record definition for the database table
typedef struct PlaylistData {
  int playlist_id;
  int next_track;
  int play_count;
};
PlaylistData playlist_data_buffer;
