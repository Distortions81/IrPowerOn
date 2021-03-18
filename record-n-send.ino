#define DECODE_NEC 1

#include <IRremote.h>
#include <EEPROM.h>

int IR_RECEIVE_PIN = 11;

uint32_t saveAddr = 0;
uint32_t saveCmd = 0;

bool programMode = true;
int sameCodeCount = 0;
unsigned long prevBlinkMillis = 0;
bool ledOn = false;

void setup()
{

  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  Serial.println("Reading values from EEPROM.");
  saveAddr = EEPROM.read(0);
  saveCmd = EEPROM.read(1);

  Serial.print("Addr ");
  Serial.println(saveAddr);

  Serial.print("Cmd ");
  Serial.println(saveAddr);

  if (saveCmd > 0)
  {
    programMode = false;
    Serial.println("Mode: Normal/Send code");
  }
  else
  {
    programMode = true;
    Serial.println("Mode: Program/Record");
  }

  Serial.print(F("Ready to receive IR signals at pin "));
  Serial.println(IR_RECEIVE_PIN);
}

void loop()
{
  if (programMode)
  {
    if (IrReceiver.decode())
    {

      // Print a short summary of received data
      IrReceiver.printIRResultShort(&Serial);
      if (IrReceiver.decodedIRData.protocol == UNKNOWN)
      {
        // We have an unknown protocol here, print more info
        IrReceiver.printIRResultRawFormatted(&Serial, true);
      }
      Serial.println();

      if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_AUTO_REPEAT || IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
      {
        Serial.println("repeat, not saving to EEPROM.");
      }
      else
      {
        saveAddr = IrReceiver.decodedIRData.address;
        EEPROM.write(0, saveAddr);
        saveCmd = IrReceiver.decodedIRData.command;
        EEPROM.write(1, saveCmd);

        Serial.println("Saved to EEPROM.");
      }

      IrReceiver.resume(); // Enable receiving of the next value

      /*
         Finally, check the received data and perform actions according to the received command
      */
      if (IrReceiver.decodedIRData.command == 0x10)
      {
        // do something
      }
      else if (IrReceiver.decodedIRData.command == 0x11)
      {
        // do something else
      }
    }
  }
}
