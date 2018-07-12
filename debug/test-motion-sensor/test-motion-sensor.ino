#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define SDCARD_CS 5   // already wired for us. Card chip select pin. already wired for us
#define VS1053_CS 6   // already wired for us. VS1053 chip select pin (output)
// DREQ should be an Int pin *if possible* (not possible on 32u4)
// pin 9 is also used for checking voltage. i guess we can't do that anymore :(
#define VS1053_DREQ 9 // already wired for us (but not marked on board). VS1053 Data request, ideally an Interrupt pin
#define VS1053_DCS 10 // already wired for us. VS1053 Data/command select pin (output)
#define START_PIN 11  // TODO: what pin?
#define LED_DATA 12    // TODO: what pin?
#define RED_LED 13     // already wired for us
#define SPI_MISO 22
#define SPI_MOSI 23
#define SPI_SCK 24

int ledPin = RED_LED;                // choose the pin for the LED
int inputPin = START_PIN;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected

void setup() {
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(inputPin, INPUT);     // declare sensor as input

  pinMode(VS1053_CS, INPUT_PULLUP);
  pinMode(VS1053_DCS, INPUT_PULLUP);

  Serial.begin(115200);

  delay(100);

  while (!Serial) {
    ; // wait for serial port to connect
  }

  Serial.println("Starting...");
}

void loop(){
  static unsigned long last = 0;

  if (digitalRead(inputPin) == HIGH) {
    digitalWrite(ledPin, HIGH);
    if (pirState == LOW) {
      // we have just turned on
      Serial.print("Motion detected! ");
      Serial.println(millis() - last);
      last = millis();
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
    digitalWrite(ledPin, LOW); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.print("Motion ended! ");
      Serial.println(millis() - last);
      last = millis();
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }

  delay(10);
}
