
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
