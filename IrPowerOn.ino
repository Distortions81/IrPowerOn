#define DECODE_NEC 1

#include <IRremote.h>
#include <EEPROM.h>

// --- CONSTANTS ---
//GPIO
const int IR_RECEIVE_PIN = 11;
const int ledPin =  LED_BUILTIN;
const int programButtonPin = 2;

//Modes
const int MODE_NORMAL = 0;
const int MODE_SEND_CODE 1;
const int MODE_CODE_SENT = 2;

const int MODE_PROGRAM = 10;
const int MODE_PROGRAM_COMPLETE = 11;

//Blink Rates
const long intervalProgram = 100;
const long intervalSend = 33;
const long intervalNormal = 1000;

//Send intervals
const int sendDelay = 30;
const int sendRepeats = 5;
const int sendRepeatDelay = 1;

//Poll interval, 1/100th of a second
const int pollInterval = 10;


// --- GLOBAL VARS ---
//EEPROM vars
uint32_t saveAddr = 0;
uint32_t saveCmd = 0;

//Mode
int currentMode = MODE_NORMAL;

//Code verification
int sameCodeCount = 0;

//LED Blink
unsigned long previousMillis = 0;
bool ledOn = false;

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
  sameCodeCount = 0;

  previousMillis = 0;
  ledOn = false;
  digitalWrite(ledPin, LOW);
}

void goProgramMode() {
  programMode = MODE_PROGRAM;
  clearTemp();
  Serial.println("Entering program mode.");

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.print(F("Enabling IR receiver on pin "));
  Serial.println(IR_RECEIVE_PIN);
}

void goNormalMode() {
  programMode = MODE_NORMAL;
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

void goCompleteMode() {
  programMode = MODE_PROGRAM_COMPLETE;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW );
  ledOn = false;

  Serial.println("Entering complete mode.");

  IrReceiver.stop();
  Serial.println("Disabling IR receiver.");
}

void goSendMode() {
  programMode = MODE_SEND_CODE;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW );
  ledOn = false;

  Serial.println("Sending IR code.");
}

void loop() {
  //Sleep for a bit
  delay(pollInterval);

  if (currentMode == MODE_PROGRAM) {

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

      if ( IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_AUTO_REPEAT ) {
        //Serial.println("Repeat, not saving to EEPROM.");
      } else {
        //Starting
        int newAddr =  IrReceiver.decodedIRData.address;
        int newCmd = IrReceiver.decodedIRData.command;


        if ( sameCodeCount == 0 || ( newAddr == saveAddr && newCmd == saveCmd)) {
          saveAddr = IrReceiver.decodedIRData.address;
          saveCmd = IrReceiver.decodedIRData.command;
          if ( sameCodeCount > 0 ) {
            Serial.println("Got same code, good!");
          }
          sameCodeCount++;
        } else {
          saveAddr = 0;
          saveCmd = 0;
          sameCodeCount = 0;
          Serial.println("Got different code, starting over.");
        }

        //After the same code a few times in a row, we will assume we got the code okay
        if ( sameCodeCount > 5 ) {
          EEPROM.write(0, newAddr);
          EEPROM.write(1, newCmd);
          Serial.println("Code appears to be good, saving in EEPROM.");
          goNormalMode();
        }
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
  } else if (currentMode == MODE_NORMAL) {
    //Check if it is time to send IR code.
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= intervalProgram) {
      previousMillis = currentMillis;

      goSendMode();
    }
  }

  //Check for program button pressed
  if (digitalRead(programButtonPin) == LOW) {
    goProgramMode();
  }
}
