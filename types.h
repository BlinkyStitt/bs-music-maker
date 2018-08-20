// Workaround from http://playground.arduino.cc/Code/Enum

#define MAX_PLAYLIST_TRACKS 255

enum BatteryStatus : byte { BATTERY_DEAD, BATTERY_LOW, BATTERY_OK, BATTERY_FULL };

typedef struct {
  unsigned long database_id = 0;

  char directory[14]; // /8.3\0

  unsigned int num_tracks = 0;
  char tracks[MAX_PLAYLIST_TRACKS][13];  // 8.3\0

  unsigned int next_track = 0;
  unsigned int play_count = 0;
  unsigned int repeat = 0;
} Playlist;

// Record definition for the database table
typedef struct {
  unsigned long playlist_id;
  unsigned int next_track;
  unsigned int play_count;
} PlaylistData;
