
void createTable() {
  DEBUG_PRINT("Creating table... ");
  // create table starting at address 0
  db.create(0, TABLE_SIZE, (unsigned int)sizeof(PlaylistData));
  DEBUG_PRINTLN("DONE");
}

void setupDatabase() {
  // open database if it exists, create database if it doesn't
  if (SD.exists(db_name)) {
    db_file = SD.open(db_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, especially after cold start
    // Let's try one more time
    if (!db_file) {
      db_file = SD.open(db_name, FILE_WRITE);
    }

    if (db_file) {
      DEBUG_PRINT("Opening current table... ");
      EDB_Status result = db.open(0);
      if (result == EDB_OK) {
        DEBUG_PRINTLN("DONE");
      } else {
        DEBUG_PRINTLN("ERROR");
        DEBUG_PRINT("Did not find database in the file ");
        DEBUG_PRINTLN(db_name);

        createTable();
      }
    } else {
      DEBUG_PRINT("Could not open file ");
      DEBUG_PRINTLN(db_name);
      // TODO: loop here since we can't proceed?
      return;
    }
  } else {
    db_file = SD.open(db_name, FILE_WRITE);

    createTable();
  }

  recordLimit();
  countRecords();

  closeDatabase();
}

bool openDatabase() {
  db_file = SD.open(db_name, FILE_WRITE);

  if (db_file) {
    return true;
  } else {
    DEBUG_PRINT("Could not open file ");
    DEBUG_PRINTLN(db_name);
    return false;
  }
}

void closeDatabase() { db_file.close(); }

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
