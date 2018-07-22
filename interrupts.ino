/*
with help from:
- https://github.com/ProjectsByJRP/M0_Sleep_External_Int/blob/master/sleep_with_ext_int_pin6.ino (BUT NOT PIN 6!)
- https://github.com/GabrielNotman/AutonomoTesting/blob/master/Interrupts/SleepChangeInterrupt/SleepChangeInterrupt.ino
- https://github.com/cavemoa/Feather-M0-Adalogger/blob/master/SimpleSleepUSB/SimpleSleepUSB.ino#L36
*/

void setupInterrupts() {
  // Attach the interrupt and set the wake flag
  attachInterrupt(digitalPinToInterrupt(START_PIN), ISR, RISING);

  // Set the XOSC32K to run in standby (needed for RISING)
  SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;

  // Configure EIC to use GCLK1 which uses XOSC32K
  // This has to be done after the first call to attachInterrupt()
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_EIC) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;

  // Set sleep mode
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
}

void ISR() {
  // TODO: is there anything we actually want to do during the interrupt?
}

void sleep() {
#ifdef DEBUG
  DEBUG_PRINTLN("Sleeping...");

  Serial.end();
  USBDevice.detach(); // Safely detach the USB prior to sleeping
#endif

  // Enter sleep mode. this should save us power
  __WFI(); // Wait for interrupt

  // we are awake!
#ifdef DEBUG
  USBDevice.attach(); // Re-attach the USB, audible sound on windows machines

  // Simple indication of being awake
  digitalWrite(RED_LED, HIGH); // turn the LED on
  delay(100);
  digitalWrite(RED_LED, LOW); // turn the LED off
  delay(100);
  digitalWrite(RED_LED, HIGH); // turn the LED on
  delay(100);
  digitalWrite(RED_LED, LOW); // turn the LED off

  // TODO: might need more of a delay here for serial to work well

  Serial.begin(115200);

  delay(1000);
  while (!Serial) {
    ; // wait for serial port to connect so we can watch all prints during setup
  }

  DEBUG_PRINTLN("Awake!");
#endif
}
