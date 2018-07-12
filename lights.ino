/* lights */

void setupLights() {
  DEBUG_PRINT("Setting up lights... ");

  // https://learn.adafruit.com/adafruit-feather-m0-basic-proto/power-management
  // "While you can get 500mA from it, you can't do it continuously from 5V as
  // it will overheat the regulator."
  // TODO: tune this
  FastLED.setMaxPowerInVoltsAndMilliamps(3.3, 400);

  // TODO: something is wrong about this. i'm getting SPI errors
  FastLED.addLeds<LED_CHIPSET, LED_DATA>(leds, num_LEDs).setCorrection(TypicalSMD5050);

  FastLED.setBrightness(default_brightness);
  FastLED.clear();
  FastLED.show();

  DEBUG_PRINTLN("done.");
}

void updateLights(bool& lights_on) {
#ifdef DEBUG
  static unsigned long last_frame = 0;
#endif

  // update the led array every frame
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {
    if (lights_on) {
      // TODO: if not motion activated and config has a time limit for lights, set lights_on = false

      pride();
    } else {
      // TODO: is this fast if all the lights are already dimmed?
      fadeToBlackBy(leds, num_LEDs, LED_FADE_RATE);
    }

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

    DEBUG_PRINT(F(" | Motion="));
    DEBUG_PRINT(digitalRead(START_PIN));

    DEBUG_PRINT(F(" | Stopped="));
    DEBUG_PRINT(musicPlayer.stopped());

    DEBUG_PRINT(F(" | ms since last frame="));
    DEBUG_PRINTLN(millis() - last_frame);

    last_frame = millis();
#endif

    // display the colors
    FastLED.show();
  }

  if (lights_on) {
    // change g_hue every 3 frames
    EVERY_N_MILLISECONDS(3 * 1000 / frames_per_second) { g_hue++; }
  }
}
