/* lights */

// TODO: split this into two functions. one that can run before config, and one that can run after. run the former before serial connects
void setupLights() {
  DEBUG_PRINT("Setting up lights... ");

  pinMode(LED_DATA, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(RED_LED, LOW);

  // TODO: seed fastled random?


  // NOTE! It isn't safe to plug the neopixels to the USB line because that might actually be 6V coming from the panel
  // we are running it off the 3.3 amp regulator instead
  // https://learn.adafruit.com/adafruit-feather-m0-basic-proto/power-management
  // "While you can get 500mA from it, you can't do it continuously from 5V as it will overheat the regulator."
  // TODO: include radio_power instead of hard coding 120 (it is 120mA when radio_power=20)
  // TODO: we won't use the radio at full power and SD at full power at the same time. so tune this
  // TODO: maybe run it without lights to see max power draw and subtract that from 500
  // we have to leave room for the amp
  // TODO: tune this! we also want to make sure we conserve battery so we might want to turn this down more
  FastLED.setMaxPowerInVoltsAndMilliamps(3.3, 200);

  // neopixel
  FastLED.addLeds<LED_CHIPSET, LED_DATA>(leds, num_LEDs).setCorrection(TypicalSMD5050);

  // apa102
  //FastLED.addLeds<LED_CHIPSET, LED_DATA, LED_CLOCK, LED_MODE>(leds, num_LEDs).setCorrection(TypicalSMD5050);

  FastLED.setBrightness(default_brightness);

  FastLED.clear();
  FastLED.show();

  DEBUG_PRINTLN("done.");
}

void lightPattern() {
  static const int network_LEDs = num_LEDs * 3; // spread the raninbow out
  static const int peer_shift = 0;
  static const int ms_per_led = 4 * 1000 / frames_per_second;   // 4 frames
  static int shift;

  static unsigned int distance = num_LEDs / 6;  // max(min_distance, max_distance / (1+nearby_peers));  // min 5, max numLEDs + a few for fade rate
  // TODO: change distance based on how many peers are nearby?
  // todo: fade slower (32) for larger distances
  fadeLightBy(leds, num_LEDs, 24);

  // shift the pattern based on peer id and then shift more slowly over time
  shift = peer_shift + millis() / ms_per_led;

  // i is our local ids for lights
  for (int i = 0; i < num_LEDs; i++) {
    int network_i = (i + shift) % network_LEDs;

    // light up every Nth light. the others will fade
    if (network_i % distance == 0) {
      // TODO: use a color pallet?
      // TODO: this spreads the whole rainbow across all 4. do we want to change color slower than that?
      // TODO: do something with saturation, too?

      // TODO: without the constrain, it sometimes crashed. it shouldn't be needed if i did the map right
      // TODO: i think maybe changing the type to byte would be a better fix since -1 or 256 should wrap
      int color_value = constrain(map(network_i, 0, network_LEDs - 1, 0, 255), 0, 255);

      // TODO: add or blend instead of set?
      leds[i] = CHSV(color_value, 230, 255);
    }
  }
}

void updateLights() {
  static unsigned long last_frame = 0;

  // update the led array every frame
  EVERY_N_MILLISECONDS(1000 / frames_per_second) {
    if (g_lights_on) {
      lightPattern();
    } else {
      fadeToBlackBy(leds, num_LEDs, 16);
    }

    #ifdef DEBUG
      // debugging lights

      int millis_wrapped = millis() % 10000;
      if (millis_wrapped < 1000) {
        DEBUG_PRINT(F(" "));

        if (millis_wrapped < 100) {
          DEBUG_PRINT(F(" "));

          if (millis_wrapped < 10) {
            DEBUG_PRINT(F(" "));
          }
        }
      }
      DEBUG_PRINT(millis_wrapped);

      DEBUG_PRINT(F(": "));
      for (int i = 0; i < num_LEDs; i++) {
        if (leds[i]) {
          // TODO: better logging?
          DEBUG_PRINT(F("X"));
        } else {
          DEBUG_PRINT(F("O"));
        }
      }

      if (!sd_setup) {
        DEBUG_PRINT(F(" | !SD"));
      }

      if (!config_setup) {
        DEBUG_PRINT(F(" | !Conf"));
      }

      DEBUG_PRINT(" | ");
      freeMemory(false);

      DEBUG_PRINT(" | Next Track=");
      DEBUG_PRINT(g_next_track);

      DEBUG_PRINT(" | Music Stopped=");
      DEBUG_PRINT(g_music_stopped);

      DEBUG_PRINT(" | Motion=");
      DEBUG_PRINT(digitalRead(START_PIN));

      DEBUG_PRINT(F(" | ms since last frame="));
      DEBUG_PRINTLN(millis() - last_frame);

      last_frame = millis();
    #endif

    // display the colors
    FastLED.show();
  }
}
