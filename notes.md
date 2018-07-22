TODO:
- loadPlaylists is broken. the database is corrupted
- are pins 11 and 12 good?
- move LED_FADE_RATE to sd card
- IniConfig doesn't like unsigned ints
- document playlist_data_buffer
- configure volume off SD (constrain 0-255)
- what table size?

TODO MOTION_ACTIVATED:
- it plays a whole song before turning off. should we tie it more to the PIR? can we pause and sleep?
- change counting of off frames. just check the brightness of all the lights instead?

TODO night sounds:
- make lights optional
- use bounce library instead of simple digitalRead for button?

TODO v2:
- instead of hard coding NUM_PLAYLISTS, loop over directories to create playlists instead

debug/test-motion-sensor:
- pir sensor and music wing don't get along. https://forums.adafruit.com/viewtopic.php?f=19&t=138121
