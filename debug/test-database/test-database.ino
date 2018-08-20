#define DEBUG
#define DEBUG_SERIAL_WAIT
#include <bs_debug.h>

#include <EDB.h>
#include <SD.h>
#include <SPI.h>

#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define SDCARD_CS 5 // already wired for us. Card chip select pin. already wired for us
#define VS1053_CS 6 // already wired for us. VS1053 chip select pin (output)
// DREQ should be an Int pin *if possible* (not possible on 32u4)
// pin 9 is also used for checking voltage. i guess we can't do that anymore :(
#define VS1053_DREQ 9 // already wired for us (but not marked on board). VS1053 Data request, ideally an Interrupt pin
#define VS1053_DCS 10 // already wired for us. VS1053 Data/command select pin (output)
#define START_PIN 11  // TODO: what pin?
#define LED_DATA 12   // TODO: what pin?
#define RED_LED 13    // already wired for us
#define SPI_MISO 22
#define SPI_MOSI 23
#define SPI_SCK 24

#define TABLE_SIZE 8192

// The number of demo records that should be created.  This should be less
// than (TABLE_SIZE - sizeof(EDB_Header)) / sizeof(logEvent).  If it is higher,
// operations will return EDB_OUT_OF_RANGE for all records outside the usable range.
#define RECORDS_TO_CREATE 10

char *db_name = "/edb_test.db";
File my_file;

// Arbitrary record definition for this table.
// This should be modified to reflect your record needs.
struct LogEvent {
  int id;
  int temperature;
} logEvent;

// The read and write handlers for using the SD Library
// Also blinks the led while writing/reading
inline void writer(unsigned long address, const byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);
  my_file.seek(address);
  my_file.write(data, recsize);
  my_file.flush();
  digitalWrite(RED_LED, LOW);
}

inline void reader(unsigned long address, byte *data, unsigned int recsize) {
  digitalWrite(RED_LED, HIGH);
  my_file.seek(address);
  my_file.read(data, recsize);
  digitalWrite(RED_LED, LOW);
}

// Create an EDB object with the appropriate write and read handlers
EDB db(&writer, &reader);

bool openDatabase() {
  DEBUG_PRINT("Opening database... ");

  my_file = SD.open(db_name, FILE_WRITE);

  if (my_file) {
    DEBUG_PRINTLN("DONE.");
    return true;
  } else {
    DEBUG_PRINT("Could not open file ");
    DEBUG_PRINTLN(db_name);
    return false;
  }
}

void closeDatabase() {
  DEBUG_PRINT("Closing database...");
  my_file.close();
  DEBUG_PRINTLN("DONE.");
}

void setup() {
  debug_serial(115200, 2000);

  DEBUG_PRINTLN(F("Setting up..."));

  randomSeed(analogRead(0));

  while (!SD.begin(SDCARD_CS)) {
    DEBUG_PRINTLN(F("SD failed, or not present. Retrying in 1 second..."));
    delay(1000);
  }
  delay(150); // SD sometimes takes some time to wake up
  DEBUG_PRINTLN("SD OK!");

  DEBUG_PRINTLN(F("Starting..."));

  if (SD.exists(db_name)) {
    my_file = SD.open(db_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, especially after cold start
    // Let's try one more time
    if (!my_file) {
      my_file = SD.open(db_name, FILE_WRITE);
    }

    if (my_file) {
      Serial.print("Opening current table... ");
      EDB_Status result = db.open(0);
      if (result == EDB_OK) {
        Serial.println("DONE");
      } else {
        Serial.println("ERROR");
        Serial.println("Did not find database in the file " + String(db_name));
        Serial.print("Creating new table... ");
        db.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
        Serial.println("DONE");
        return;
      }
    } else {
      Serial.println("Could not open file " + String(db_name));
      return;
    }
  } else {
    Serial.print("Creating table... ");
    // create table at with starting address 0
    my_file = SD.open(db_name, FILE_WRITE);
    db.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
    Serial.println("DONE");
  }

  recordLimit();
  countRecords();
  createRecords(RECORDS_TO_CREATE);
  countRecords();
  selectAll();

  // i think re-opening a closed database is broken
  closeDatabase();
  openDatabase();

  recordLimit();
  countRecords();
  createRecords(RECORDS_TO_CREATE);
  countRecords();
  selectAll();

  // i think re-opening a closed database is broken
  closeDatabase();
  openDatabase();

  deleteOneRecord(RECORDS_TO_CREATE / 2);
  countRecords();
  selectAll();
  appendOneRecord(RECORDS_TO_CREATE + 1);
  countRecords();
  selectAll();
  insertOneRecord(RECORDS_TO_CREATE / 2);
  countRecords();
  selectAll();
  updateOneRecord(RECORDS_TO_CREATE);
  selectAll();
  countRecords();
  deleteAll();
  Serial.println("Use insertRec() and deleteRec() carefully, they can be slow");
  countRecords();
  for (int i = 1; i <= 20; i++)
    insertOneRecord(1); // inserting from the beginning gets slower and slower
  countRecords();
  for (int i = 1; i <= 20; i++)
    deleteOneRecord(1); // deleting records from the beginning is slower than from the end
  countRecords();

  closeDatabase();
}

void loop() {}

// utility functions

void recordLimit() {
  Serial.print("Record Limit: ");
  Serial.println(db.limit());
}

void deleteOneRecord(int recno) {
  Serial.print("Deleting recno: ");
  Serial.println(recno);
  db.deleteRec(recno);
}

void deleteAll() {
  Serial.print("Truncating table... ");
  db.clear();
  Serial.println("DONE");
}

void countRecords() {
  Serial.print("Record Count: ");
  Serial.println(db.count());
}

void createRecords(int num_recs) {
  Serial.print("Creating Records... ");
  for (int recno = 1; recno <= num_recs; recno++) {
    logEvent.id = recno;
    logEvent.temperature = random(1, 125);
    EDB_Status result = db.appendRec(EDB_REC logEvent);
    if (result != EDB_OK)
      printError(result);
  }
  Serial.println("DONE");
}

void selectAll() {
  for (int recno = 1; recno <= db.count(); recno++) {
    EDB_Status result = db.readRec(recno, EDB_REC logEvent);
    if (result == EDB_OK) {
      Serial.print("Recno: ");
      Serial.print(recno);
      Serial.print(" ID: ");
      Serial.print(logEvent.id);
      Serial.print(" Temp: ");
      Serial.println(logEvent.temperature);
    } else
      printError(result);
  }
}

void updateOneRecord(int recno) {
  Serial.print("Updating record at recno: ");
  Serial.print(recno);
  Serial.print("... ");
  logEvent.id = 1234;
  logEvent.temperature = 4321;
  EDB_Status result = db.updateRec(recno, EDB_REC logEvent);
  if (result != EDB_OK)
    printError(result);
  Serial.println("DONE");
}

void insertOneRecord(int recno) {
  Serial.print("Inserting record at recno: ");
  Serial.print(recno);
  Serial.print("... ");
  logEvent.id = recno;
  logEvent.temperature = random(1, 125);
  EDB_Status result = db.insertRec(recno, EDB_REC logEvent);
  if (result != EDB_OK)
    printError(result);
  Serial.println("DONE");
}

void appendOneRecord(int id) {
  Serial.print("Appending record... ");
  logEvent.id = id;
  logEvent.temperature = random(1, 125);
  EDB_Status result = db.appendRec(EDB_REC logEvent);
  if (result != EDB_OK)
    printError(result);
  Serial.println("DONE");
}

void printError(EDB_Status err) {
  Serial.print("ERROR: ");
  switch (err) {
  case EDB_OUT_OF_RANGE:
    Serial.println("Recno out of range");
    break;
  case EDB_TABLE_FULL:
    Serial.println("Table full");
    break;
  case EDB_OK:
  default:
    Serial.println("OK");
    break;
  }
}
