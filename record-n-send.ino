#define DECODE_NEC 1

#include <IRremote.h>
#include <EEPROM.h>

//GPIO
const int IR_RECEIVE_PIN = 11;
const int ledPin =  LED_BUILTIN;
const int programButtonPin = 2;

//EEPROM vars
uint32_t saveAddr = 0;
uint32_t saveCmd = 0;

uint32_t lastAddr = 0;
uint32_t lastCmd = 0;

//Mode, same code rev count
bool programMode = false;
int sameCodeCount = 0;

//LED Blink
unsigned long previousMillis = 0;
bool ledOn = false;
const long intervalProgram = 250;

//Normal mode: Code send delay, repeats, repeat delays
const int sendDelay = 30;
const int sendRepeats = 5;
const int sendRepeatDelay = 1;

//Poll interval, 1/10th of a second
const int pollInterval = 100;

void setup() {

  //Setup serial, IR recv, and status LED.
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting...");

  pinMode(ledPin, OUTPUT);
  pinMode(programButtonPin, INPUT_PULLUP);


  saveAddr = EEPROM.read(0);
  saveCmd = EEPROM.read(1);

  if (saveCmd > 0) {
    goNormalMode();

  } else {
    goProgramMode();

  }
}

void clearTemp() {
  saveCmd = 0;
  saveAddr = 0;
  lastCmd = 0;
  lastAddr = 0;
  sameCodeCount = 0;

  previousMillis = 0;
}

void goProgramMode() {
  programMode = true;
  clearTemp();
  Serial.println("Entering program mode.");

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.print(F("Enabling IR receiver on pin "));
  Serial.println(IR_RECEIVE_PIN);
}

void goNormalMode() {
  programMode = false;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW );
  ledOn = false;

  Serial.println("Entering normal mode.");
  Serial.println("Reading values from EEPROM.");
  saveAddr = EEPROM.read(0);
  saveCmd = EEPROM.read(1);

  Serial.print("Addr ");
  Serial.println(saveAddr);
  Serial.print("Cmd ");
  Serial.println(saveCmd);

  IrReceiver.stop();
  Serial.println("Disabling IR receiver.");
}

void loop() {
  //Sleep for a bit
  delay(pollInterval);
  
  if (programMode) {

    //Check if it is time to blink status LED, to indicate program mode.
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= intervalProgram) {
      previousMillis = currentMillis;

      //Blink LED
      if (ledOn ) {
        ledOn = false;
        digitalWrite(ledPin, LOW );
      } else {
        ledOn = true;
        digitalWrite(ledPin, HIGH );
      }
    }

    if (IrReceiver.decode()) {

      Serial.println("");
      if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
        Serial.println("Unknown protocol, ignored.");
      } else {
        Serial.print("Addr ");
        Serial.println(IrReceiver.decodedIRData.address);

        Serial.print("Cmd ");
        Serial.println(IrReceiver.decodedIRData.command);
      }

      if ( IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_AUTO_REPEAT || IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT ) {
        Serial.println("Repeat, not saving to EEPROM.");
      } else {
        saveAddr = IrReceiver.decodedIRData.address;
        saveCmd = IrReceiver.decodedIRData.command;

        Serial.println("Saved to EEPROM.");
      }

      IrReceiver.resume(); // Enable receiving of the next value

      /*
         Finally, check the received data and perform actions according to the received command
      */
      if (IrReceiver.decodedIRData.command == 0x10) {
        // do something
      } else if (IrReceiver.decodedIRData.command == 0x11) {
        // do something else
      }
    }
  } else {

    //Check for program button pressed
    if (digitalRead(programButtonPin) == LOW) {
      goProgramMode();
    }
  }
}
