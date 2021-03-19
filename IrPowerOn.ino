#define DECODE_NEC 1

#include <IRremote.h>
#include <EEPROM.h>

// --- CONSTANTS ---
//GPIO
const int IR_RECEIVE_PIN = 11;
const int ledPin = LED_BUILTIN;
const int programButtonPin = 2;

//Modes
const int MODE_NORMAL = 0;
const int MODE_SEND_CODE = 1;
const int MODE_CODE_SENT = 2;

const int MODE_PROGRAM = 10;
const int MODE_PROGRAM_COMPLETE = 11;

//Blink Rates
const long intervalProgram = 100;
const long intervalNormal = 990;

//Send intervals
const int sendDelay = 30 * 1000;
const int sendRepeats = 10;
const int sendRepeatDelay = 1000;

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

//LED blink millis
unsigned long previousMillisLED = 0;
bool ledOn = false;

//Send repeat millis
unsigned long previousMillisSend = 0;

//Count number of times code sent
int codeSentCount = 0;

void setup()
{
  delay(1000);
  //Setup serial, IR recv, and status LED.
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting...");

  pinMode(ledPin, OUTPUT);
  pinMode(programButtonPin, INPUT_PULLUP);

  saveAddr = EEPROM.read(0);
  saveCmd = EEPROM.read(1);

  if (saveCmd > 0)
  {
    goNormalMode();
  }
  else
  {
    goProgramMode();
  }
}

void clearTemp()
{
  saveCmd = 0;
  saveAddr = 0;
  sameCodeCount = 0;
  codeSentCount = 0;

  previousMillisLED = 0;
  previousMillisSend = 0;

  ledOn = false;
  digitalWrite(ledPin, LOW);
}

void goProgramMode()
{
  currentMode = MODE_PROGRAM;
  clearTemp();
  Serial.println("Entering program mode.");

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.print(F("Enabling IR receiver on pin "));
  Serial.println(IR_RECEIVE_PIN);
}

void goNormalMode()
{
  currentMode = MODE_NORMAL;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW);
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

  Serial.print("Sending IR code in ");
  Serial.print(sendDelay/1000);
  Serial.println(" seconds!");
}

void goCompleteMode()
{
  currentMode = MODE_PROGRAM_COMPLETE;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW);
  ledOn = false;

  Serial.println("Entering program-complete mode.");
}

void goSendMode()
{
  currentMode = MODE_SEND_CODE;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW);
  ledOn = false;
}

void goSendComplete()
{
  currentMode = MODE_CODE_SENT;
  clearTemp();
  Serial.println("IR code send complete, sleeping...");

  //Clear status LED.
  digitalWrite(ledPin, LOW);
  ledOn = false;
}

void loop()
{
  //Sleep for a bit
  delay(pollInterval);

  if (currentMode == MODE_PROGRAM)
  {

    //Check if it is time to blink status LED, to indicate program mode.
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisLED >= intervalProgram)
    {
      previousMillisLED = currentMillis;

      //Blink LED
      if (ledOn)
      {
        ledOn = false;
        digitalWrite(ledPin, LOW);
      }
      else
      {
        ledOn = true;
        digitalWrite(ledPin, HIGH);
      }
    }

    if (IrReceiver.decode())
    {

      Serial.println("");
      if (IrReceiver.decodedIRData.protocol == UNKNOWN)
      {
        Serial.println("Unknown protocol, ignored.");
      }
      else
      {
        Serial.print("Addr ");
        Serial.println(IrReceiver.decodedIRData.address);

        Serial.print("Cmd ");
        Serial.println(IrReceiver.decodedIRData.command);
      }

      if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_AUTO_REPEAT)
      {
        //Serial.println("Repeat-code, ignoring...");
      }
      else
      {
        //Starting
        int newAddr = IrReceiver.decodedIRData.address;
        int newCmd = IrReceiver.decodedIRData.command;

        if (sameCodeCount == 0 || (newAddr == saveAddr && newCmd == saveCmd))
        {
          saveAddr = IrReceiver.decodedIRData.address;
          saveCmd = IrReceiver.decodedIRData.command;
          if (sameCodeCount > 0)
          {
            Serial.println("Got same code, good!");
          }
          sameCodeCount++;
        }
        else
        {
          saveAddr = 0;
          saveCmd = 0;
          sameCodeCount = 0;
          Serial.println("Got different code, starting over.");
        }

        //After the same code a few times in a row, we will assume we got the code okay
        if (sameCodeCount > 5)
        {
          EEPROM.write(0, newAddr);
          EEPROM.write(1, newCmd);
          Serial.println("Code appears to be good, saving in EEPROM.");
          goCompleteMode();
          return;
        }
      }

      IrReceiver.resume(); // Enable receiving of the next value
    }
  }
  else if (currentMode == MODE_NORMAL)
  {
    //Check if it is time to blink status LED, to indicate count down
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisLED >= intervalNormal)
    {
      previousMillisLED = currentMillis;
      digitalWrite(ledPin, HIGH);
      delay(10);

      ledOn = false;
      digitalWrite(ledPin, LOW);
    }

    //Check if it is time to send IR code.
    currentMillis = millis();
    if (currentMillis - previousMillisSend >= sendDelay)
    {
      previousMillisSend = currentMillis;
      goSendMode();
    }
  }
  else if (currentMode == MODE_SEND_CODE)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisSend >= sendRepeatDelay)
    {
      previousMillisSend = currentMillis;
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      ledOn = true;
      //SEND IR CODE HERE
      Serial.println("Sending IR Code...");
      codeSentCount++;
    }
    if ( codeSentCount > sendRepeats )
    {
      goSendComplete();
    }
  }
  else if (currentMode == MODE_CODE_SENT)
  {
    //Nothing to do but look for program button, sleep
    delay(100);
  }

  //Check for program button pressed
  if (currentMode != MODE_PROGRAM && digitalRead(programButtonPin) == LOW)
  {
    goProgramMode();
  }
}
