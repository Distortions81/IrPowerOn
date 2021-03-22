//Written by Carl Frank Otto III
//Version 0.0.2-03202021-0208p
#include <EEPROM.h>

//SEEED IR library
#include <IRLibRecvPCI.h>
#include <IRLibSendBase.h>
#include <IRLib_HashRaw.h>

//IR Code storage (RAM)
#define MAX_STORE_SIZE 255
uint16_t storedCodes[MAX_STORE_SIZE];
uint8_t storedCodesLength = 0;

//IR pins/settings
#define IR_RX_PIN 2
#define IR_TX_PIN 3 //Doesn't set in lib
#define IR_FREQ 36

//Need some play in the timings because of jitter
//freq / 1.3 == microseconds per cycle
//Last number is the allowance percent, 1.5 == 50%, so about 34 microseconds
#define ALLOWED_JITTER ((IR_FREQ / 1.3) * 1.5)
//Number of accepted codes needed to save
#define goodCodesNeeded 5

IRrecvPCI myReceiver(IR_RX_PIN);
IRsendRaw mySender;

// --- CONSTANTS / DEFINES ---
//GPIO
#define ledPin LED_BUILTIN
#define programButtonPin 4

//Modes
#define MODE_NORMAL 0
#define MODE_SEND_CODE 1
#define MODE_CODE_SENT 2

#define MODE_PROGRAM 10
#define MODE_PROGRAM_COMPLETE 11

//Blink Rates
#define intervalProgram 100
#define intervalNormal 990
#define interbalpButton 500

//Send intervals
#define sendDelay 3000
#define sendRepeats 0
#define sendRepeatDelay 50

// --- GLOBAL VARS ---
//Mode
int currentMode = MODE_NORMAL;

//Code verification
int sameCodeCount = 0;

//LED blink millis
unsigned long previousMillisLED = 0;
bool ledOn = false;

//Send repeat millis
unsigned long previousMillisSend = 0;

//Send repeat millis
unsigned long prevpButtonMillis = 0;

//Count number of times code sent
int codeSentCount = 0;

//Previous button state
int prevpButtonState = LOW;

void writeUnsignedIntIntoEEPROM(int address, uint16_t number)
{
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

uint16_t readUnsignedIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void setup()
{
  //Save current button state so we can detect a change
  pinMode(ledPin, OUTPUT);
  pinMode(programButtonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);
  ledOn = false;

  //Solves double-reset oddness, and lets input pullup stabilize.
  delay(500);
  prevpButtonState = digitalRead(programButtonPin);

  //Setup serial, and IR send
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting...");

int i;
  for (i = 0; i < MAX_STORE_SIZE; i++)
  {
    storedCodes[i] = 0;
  }

  //Setup IR send pin
  Serial.print(F("Ready to send IR signals at pin "));
  Serial.println(IR_RX_PIN);

  //READ EEPROM
  storedCodesLength = EEPROM.read(0);
  if (storedCodesLength > 0)
  {
    Serial.print("EEPROM read len: ");
    Serial.println(storedCodesLength);
    for (i = 0; i <= storedCodesLength; i++)
    {
      storedCodes[i] = readUnsignedIntFromEEPROM((i+1) * 2);

      Serial.print(storedCodes[i]);
      Serial.print(", ");

      if (i % 20 == 0)
      {
        Serial.println("");
      }
    }
    Serial.println("END");
    goNormalMode();
    return;
  }

  goProgramMode();
}

void clearTemp()
{
  sameCodeCount = 0;
  codeSentCount = 0;

  previousMillisLED = 0;
  previousMillisSend = 0;
  prevpButtonMillis = 0;

  ledOn = false;
  digitalWrite(ledPin, LOW);
}

void goProgramMode()
{
  currentMode = MODE_PROGRAM;
  clearTemp();
  Serial.println("Entering program mode.");

  myReceiver.enableIRIn(); // Start the receiver
  Serial.print("Ready to receive IR signals on pin ");
  Serial.println(IR_RX_PIN);
}

void goNormalMode()
{
  currentMode = MODE_NORMAL;
  clearTemp();

  //Clear status LED.
  digitalWrite(ledPin, LOW);
  ledOn = false;

  Serial.println("Entering normal mode.");
  //Get data from EEPROM HERE

  Serial.print("Sending IR code in ");
  Serial.print(sendDelay / 1000);
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

  int curpButtonState = digitalRead(programButtonPin);
  //Check for program button state changes
  if (currentMode != MODE_PROGRAM && curpButtonState != prevpButtonState)
  {
    //Update state
    prevpButtonState = curpButtonState;

    unsigned long currentMillis = millis();
    //Check last time we triggered program mode
    if (currentMillis - prevpButtonMillis >= interbalpButton)
    {
      prevpButtonMillis = currentMillis;
      goProgramMode();
    }
  }

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

    if (myReceiver.getResults())
    {

      //Check if code is really short, and probably a repeat code... or if it is too long to store.
      if (recvGlobal.recvLength > 4 && recvGlobal.recvLength < MAX_STORE_SIZE)
      {
        storedCodesLength = recvGlobal.recvLength;

        Serial.println("");
        Serial.print("Length: ");
        Serial.print(recvGlobal.recvLength);
        if (sameCodeCount == 0)
        {
          Serial.println(" , Data:");
        }
        else
        {
          Serial.println(" , Diff:");
        }

        bool codeBad = false;
        int i;
        for (i = 1; i < storedCodesLength; i++)
        {
          if (sameCodeCount == 0)
          {
            storedCodes[i] = recvGlobal.recvBuffer[i];
            Serial.print(storedCodes[i]);
            Serial.print(", ");
          }
          else
          {
            //If the codes are reasonably close ( jitter ), consider them the same
            uint64_t diff = abs((int64_t)storedCodes[i] - (int64_t)recvGlobal.recvBuffer[i]);
            if (diff < ALLOWED_JITTER)
            {
              Serial.print((uint32_t)diff);
              Serial.print(", ");
            }
            else
            {
              Serial.print("BAD, ");
              codeBad = true;
            }
          }
          if (i % 20 == 0)
          {
            Serial.println("");
          }
        }
        Serial.println("END.");

        if (!codeBad)
        {
          sameCodeCount++;
          if (sameCodeCount > goodCodesNeeded)
          {
            Serial.println("Codes appear good, saving to EEPROM!");

            int i;
            EEPROM.write(0, storedCodesLength);
            for (i = 0; i <= storedCodesLength; i++)
            {
              writeUnsignedIntIntoEEPROM((i+1) * 2, storedCodes[i]);
            }
            goNormalMode();
          }
        }
        else
        {
          Serial.println("Code didn't match!!! Starting over!");
          sameCodeCount = 0;
        }
      }
      else
      {
        Serial.println("Code is invalid, or a repeat code... skipping.");
      }

      //This should get skipped once code is verified as good.
      myReceiver.enableIRIn();
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

      //SEND IR CODE
      mySender.send(storedCodes, storedCodesLength, IR_FREQ);
      Serial.println("Sending IR Code...");
      codeSentCount++;
    }
    if (codeSentCount >= sendRepeats)
    {
      goSendComplete();
    }
  }
  else if (currentMode == MODE_CODE_SENT)
  {
    //Nothing to do but look for program button, sleep
  }
}