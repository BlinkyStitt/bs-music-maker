/* lights */

void setupLights() {
  DEBUG_PRINT("Setting up lights... ");

  pinMode(LED_DATA, OUTPUT);

  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);

  // https://learn.adafruit.com/adafruit-feather-m0-basic-proto/power-management
  // "While you can get 500mA from it, you can't do it continuously from 5V as
  // it will overheat the regulator."
  FastLED.setMaxPowerInVoltsAndMilliamps(3.3, 500);

  FastLED.addLeds<LED_CHIPSET, LED_DATA>(leds, num_LEDs).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(default_brightness);
  FastLED.clear();
  FastLED.show();

  DEBUG_PRINTLN("done.");
}

void updateLights() {
  static unsigned long last_frame = 0;

  // decrease overall brightness if battery is low
  // TODO: how often should we do this?
  EVERY_N_SECONDS(120) {
    switch (checkBattery()) {
    case BATTERY_DEAD:
      // TODO: use map_float(quadwave8(millis()), 0, 256, 0.3, 0.5);
      // TODO: maybe add a red led to a strip of 8 LEDs?
      FastLED.setBrightness(default_brightness * .5);
      break;
    case BATTERY_LOW:
      FastLED.setBrightness(default_brightness * .75);
      break;
    case BATTERY_OK:
      FastLED.setBrightness(default_brightness * .90);
      break;
    case BATTERY_FULL:
      FastLED.setBrightness(default_brightness);
      break;
    }
  }

  // update the led array every frame
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {

    // TODO: turn lights off if no music is playing
    // TODO: turn lights off if configured to only be on for X minutes (lights to get to bed vs nightlight-mode)
    pride();

#ifdef DEBUG
    // debugging lights
    int ms_wrapped = millis() % 10000;
    if (ms_wrapped < 1000) {
      DEBUG_PRINT(F(" "));

      if (ms_wrapped < 100) {
        DEBUG_PRINT(F(" "));

        if (ms_wrapped < 10) {
          DEBUG_PRINT(F(" "));
        }
      }
    }
    DEBUG_PRINT(ms_wrapped);

    DEBUG_PRINT(F(": "));
    for (int i = 0; i < num_LEDs; i++) {
      if (leds[i]) {
        // TODO: better logging?
        DEBUG_PRINT(F("X"));
      } else {
        DEBUG_PRINT(F("O"));
      }
    }

    DEBUG_PRINT(F(" | ms since last frame="));
    DEBUG_PRINTLN(millis() - last_frame);

    last_frame = millis();
#endif

    // display the colors
    FastLED.show();
  }

  // change g_hue every 3 frames
  EVERY_N_MILLISECONDS(3 * 1000 / frames_per_second) { g_hue++; }
}
