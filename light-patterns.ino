void chasingRainbow() {
  static const unsigned int virtual_LEDs = num_LEDs * 4;  // show 1/4 of the rainbow
  static const unsigned int ms_per_led = 3 * 1000 / frames_per_second;   // 4 frames
  static int shift;

  static const unsigned int distance = 10;

  fadeToBlackBy(leds, num_LEDs, 96);
  //fadeLightBy(leds, num_LEDs, 64);

  // shift the pattern slowly over time
  shift = millis() / ms_per_led;

  // i is our local ids for lights
  for (int i = 0; i < num_LEDs; i++) {
    int virtual_i = (i + shift) % virtual_LEDs;

    // light up every Nth light. the others will fade
    if (virtual_i % distance == 0) {
      // TODO: use a color pallet?
      // TODO: do something with saturation, too?
      int color_value = map(virtual_i, 0, virtual_LEDs - 1, 0, 255);
      leds[i] = CHSV(color_value, 230, 255);
    }
  }
}