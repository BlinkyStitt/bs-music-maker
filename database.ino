// TODO: db writes don't seem to be happening. the database is an empty file

void createTable() {
  DEBUG_PRINT(F("Creating table... "));
  // create table starting at address 0
  db.create(0, TABLE_SIZE, (unsigned int)sizeof(playlist_data_buffer));
  DEBUG_PRINTLN(F("DONE"));
}

void setupDatabase() {
  // open database if it exists, create database if it doesn't

  // while debugging, nuke the database every time. TODO: remove this!
  if (SD.exists(db_name)) {
    DEBUG_PRINTLN("RESETTING DATABASE!");
    SD.remove(db_name);
  }

  if (SD.exists(db_name)) {
    my_file = SD.open(db_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, especially after cold start
    // Let's try one more time
    if (!my_file) {
      my_file = SD.open(db_name, FILE_WRITE);
    }

    if (my_file) {
      DEBUG_PRINT(F("Opening current table... "));
      EDB_Status result = db.open(1);
      if (result == EDB_OK) {
        DEBUG_PRINTLN(F("DONE"));
      } else {
        DEBUG_PRINTLN(F("ERROR"));
        DEBUG_PRINT(F("Did not find database in the file "));
        DEBUG_PRINTLN(db_name);

        createTable();
      }
    } else {
      DEBUG_PRINT(F("Could not open file "));
      DEBUG_PRINTLN(db_name);
      // TODO: loop here since we can't proceed?
      return;
    }
  } else {
    my_file = SD.open(db_name, FILE_WRITE);

    createTable();
  }

  closeDatabase();
}

bool openDatabase() {
  DEBUG_PRINT("Opening database... ");

  my_file = SD.open(db_name, FILE_WRITE);

  if (my_file) {
    DEBUG_PRINTLN("DONE.");

    // verbose debugging!
    recordLimit();
    countRecords();
    //selectAll();

    return true;
  } else {
    DEBUG_PRINT("Could not open file ");
    DEBUG_PRINTLN(db_name);
    return false;
  }
}

void closeDatabase() {
  DEBUG_PRINTLN("Closing database...");

  // verbose debugging
  recordLimit();
  countRecords();
  //selectAll();

  my_file.close();
  DEBUG_PRINTLN("DONE.");

}

// utility functions

void recordLimit() {
  DEBUG_PRINT("Record Limit: ");
  DEBUG_PRINTLN(db.limit());
}

void deleteOneRecord(int recno) {
  DEBUG_PRINT("Deleting recno: ");
  DEBUG_PRINTLN(recno);
  db.deleteRec(recno);
}

void deleteAll() {
  DEBUG_PRINT("Truncating table... ");
  db.clear();
  DEBUG_PRINTLN("DONE");
}

void countRecords() {
  DEBUG_PRINT("Record Count: ");
  DEBUG_PRINTLN(db.count());
}

#ifdef DEBUG

void printError(EDB_Status err) {
  DEBUG_PRINT("ERROR: ");
  switch (err) {
  case EDB_OUT_OF_RANGE:
    DEBUG_PRINTLN("Recno out of range");
    break;
  case EDB_TABLE_FULL:
    DEBUG_PRINTLN("Table full");
    break;
  case EDB_OK:
  default:
    DEBUG_PRINTLN("OK");
    break;
  }
}

#else

void printError(EDB_Status err) {}

#endif
