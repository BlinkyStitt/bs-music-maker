BatteryStatus checkBattery() {
  // TODO: do something with this
  float measuredvbat = analogRead(VBAT_PIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  /*
  DEBUG_PRINT(F("VBat: "));
  DEBUG_PRINTLN(measuredvbat);
  */

  DEBUG_PRINT(F("Battery: "));
  if (measuredvbat < 3.3) {
    DEBUG_PRINTLN(F("DEAD"));
    return BATTERY_DEAD;
  }

  if (measuredvbat < 3.7) {
    DEBUG_PRINTLN(F("LOW"));
    return BATTERY_LOW;
  }

  if (measuredvbat < 4.1) {
    DEBUG_PRINTLN(F("OK"));
    return BATTERY_OK;
  }

  DEBUG_PRINTLN(F("FULL"));
  return BATTERY_FULL;
}
