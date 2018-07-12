/* Config */

#ifdef DEBUG
// from
// https://github.com/stevemarple/IniFile/blob/master/examples/IniFileExample/IniFileExample.ino
void printErrorMessage(uint8_t e, bool eol = true) {
  DEBUG_PRINT(F("Config Error: "));

  switch (e) {
  case IniFile::errorNoError:
    DEBUG_PRINT(F("no error"));
    break;
  case IniFile::errorFileNotFound:
    DEBUG_PRINT(F("file not found"));
    break;
  case IniFile::errorFileNotOpen:
    DEBUG_PRINT(F("file not open"));
    break;
  case IniFile::errorBufferTooSmall:
    DEBUG_PRINT(F("buffer too small"));
    break;
  case IniFile::errorSeekError:
    DEBUG_PRINT(F("seek error"));
    break;
  case IniFile::errorSectionNotFound:
    DEBUG_PRINT(F("section not found"));
    break;
  case IniFile::errorKeyNotFound:
    DEBUG_PRINT(F("key not found"));
    break;
  case IniFile::errorEndOfFile:
    DEBUG_PRINT(F("end of file"));
    break;
  case IniFile::errorUnknownError:
    DEBUG_PRINT(F("unknown error"));
    break;
  default:
    DEBUG_PRINT(F("unknown error value"));
    break;
  }
  if (eol) {
    DEBUG_PRINTLN();
  }
}
#else
void printErrorMessage(uint8_t e, bool eol = true) {}
#endif

void setupConfig() {
  const size_t buffer_len = 80;
  char buffer[buffer_len];

  const char *filename = "/config.ini";

  // create this even if sd isn't setup so that the if/else below is simpler
  IniFile ini(filename);

  if (!ini.open()) {
    DEBUG_PRINTLN(F("Config does not exist."));
  } else {
    DEBUG_PRINTLN(F("Loading config... "));

    if (!ini.validate(buffer, buffer_len)) {
      DEBUG_PRINT(ini.getFilename());
      DEBUG_PRINT(F("not valid. Cannot proceed with config!"));
      printErrorMessage(ini.getError());
    } else {
      ini.getValue("global", "default_brightness", buffer, buffer_len, default_brightness);
      ini.getValue("global", "frames_per_second", buffer, buffer_len, frames_per_second);
      ini.getValue("global", "alarm_hours", buffer, buffer_len, alarm_hours);
      ini.getValue("global", "alarm_minutes", buffer, buffer_len, alarm_minutes);
      ini.getValue("global", "alarm_seconds", buffer, buffer_len, alarm_seconds);
    }

    ini.close();
  }

  DEBUG_PRINT(F("default_brightness: "));
  if (!default_brightness) {
    DEBUG_PRINT(F("(default) "));
    default_brightness = 128; // TODO: tune this
  }
  DEBUG_PRINTLN(default_brightness);

  DEBUG_PRINT(F("frames_per_second: "));
  if (!frames_per_second) {
    DEBUG_PRINT(F("(default) "));
    frames_per_second = 60; // todo: this ends up being 50
  }
  DEBUG_PRINTLN(frames_per_second);

  DEBUG_PRINT(F("goal milliseconds per frame: "));
  DEBUG_PRINTLN2(1000.0 / frames_per_second, 2);

  DEBUG_PRINT(F("alarm_hours: "));
  if (!alarm_hours) {
    DEBUG_PRINT(F("(default) "));
    alarm_hours = 2;
  }
  DEBUG_PRINTLN(alarm_hours);

  DEBUG_PRINT(F("alarm_minutes: "));
  if (!alarm_minutes) {
    DEBUG_PRINT(F("(default) "));
    alarm_minutes = 0;
  }
  DEBUG_PRINTLN(alarm_minutes);

  DEBUG_PRINT(F("alarm_seconds: "));
  if (!alarm_seconds) {
    DEBUG_PRINT(F("(default) "));
    alarm_seconds = 0;
  }
  DEBUG_PRINTLN(alarm_seconds);
}
