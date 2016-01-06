/* Pulse Generator x2 (8 Channel) */
/* Designed by: ID&F, JRC-HHMI */
/* Author: Lakshminarayanan Ramasamy PhD */
/* Principal Investigator: Marta Zlatic  */
/* Date created: April 15th 2015*/
/* Date last modified: May 5th 2015 */

#include <TimerOne.h>
#include <EEPROM.h>

// Constant definition
#define lRate 200 //Interrupt loop rate defined in microseconds
#define TPIN 12

//User defined variables
char serBuf[7], cBuf[6];
int run[8], lstate1[8], lstate2[8], nChar, cmdID, bID, out[8] = {14, 15, 16, 17, 18, 19, 20, 21}, tpSt, invOut, bitVal;
double ton1[8], toff1[8], pcount1[8], ton2[8], toff2[8], pcount2[8], duration[8], tcount[8], delayTime[8];

// USER can quickly debug whether the controller is working or NOT by probing pin 12 for square wave signal of frequency = 1/(2xlRate)
void toggleDebugPin(void)
{
  tpSt = !tpSt;
  digitalWrite(TPIN, tpSt);
}

// Timer interrupt callback
void tInt(void)
{
  toggleDebugPin();
  for (int ch = 0; ch <= 7; ch++)
  {
    if (run[ch] == 1)
    {
      tcount[ch] = tcount[ch] + lRate;
      if (tcount[ch] > (duration[ch] + delayTime[ch]))
      {
        run[ch] = 0;
        lstate1[ch] = 0;
        pcount1[ch] = 0;
        lstate2[ch] = 0;
        pcount2[ch] = 0;
        if (bitRead(invOut, ch) == 0) digitalWrite(out[ch], LOW);
        if (bitRead(invOut, ch) == 1) digitalWrite(out[ch], HIGH);
      }
      else
      {
        if (tcount[ch] > delayTime[ch])
        {
          /* Pulse level 2 */
          pcount2[ch] = pcount2[ch] + lRate;
          if (lstate2[ch] == 0)
          {
            if (pcount2[ch] > toff2[ch])
            {
              lstate2[ch] = 1;
              lstate1[ch] = 1;
              pcount2[ch] = pcount2[ch] - toff2[ch];
            }
          }
          else
          {
            if (pcount2[ch] > ton2[ch])
            {
              lstate2[ch] = 0;
              lstate1[ch] = 0;
              pcount2[ch] = pcount2[ch] - ton2[ch];
            }
          }

          /* Pulse level 1 */
          if (lstate2[ch] == 1)
          {
            pcount1[ch] = pcount1[ch] + lRate;
            if (lstate1[ch] == 0)
            {
              if (pcount1[ch] > toff1[ch])
              {
                lstate1[ch] = 1;
                pcount1[ch] = pcount1[ch] - toff1[ch];
              }
            }
            else
            {
              if (pcount1[ch] > ton1[ch])
              {
                lstate1[ch] = 0;
                pcount1[ch] = pcount1[ch] - ton1[ch];
              }
            }
          }
          else
          {
            lstate1[ch] = 0;
            pcount1[ch] = 0;
          }
          if (bitRead(invOut, ch) == 0)
          {
            if (lstate1[ch] == 1) digitalWrite(out[ch], HIGH);
            if (lstate1[ch] == 0) digitalWrite(out[ch], LOW);
          }
          else
          {
            if (lstate1[ch] == 1) digitalWrite(out[ch], LOW);
            if (lstate1[ch] == 0) digitalWrite(out[ch], HIGH);
          }
        }
      }
    }
  }
}




void setup()
{
  Serial.setTimeout(100);
  Serial.begin(115200);
  invOut = EEPROM.read(0);

  // Default pulse settings
  for (int ch = 0; ch < 8; ch++)
  {
    run[ch] = 0;
    lstate1[ch] = 1;
    ton1[ch] = 10000;
    toff1[ch] = 10000;
    lstate2[ch] = 1;
    ton2[ch] = 100000;
    toff2[ch] = 100000;
    duration[ch] = 300000;
    delayTime[ch] = 0;
  }

  // Setting PIN direction to "OUTPUT"
  pinMode(TPIN, OUTPUT);

  // Set channels 1 to 8 direction as Output
  for (int ch = 0; ch < 8; ch++)
  {
    pinMode(out[ch], OUTPUT);
    if (bitRead(invOut, ch) == 1)digitalWrite(out[ch], HIGH);
    else digitalWrite(out[ch], LOW);
  }

  Timer1.attachInterrupt(tInt);
  Timer1.initialize(lRate); // Calls every 100 us
  Timer1.start();
}


void loop()
{
  // Waits indefinetly, receives the command from the PC or from the main controller and executes the command.
  int ch;
  nChar = Serial.available();
  if (nChar > 1)
  {
    bID = Serial.read();
    if (bID == 255)
    {
      cmdID = Serial.read();
      switch (cmdID)
      {
        case 'a': // TON1
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          ton1[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'b': // TOFF1
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          toff1[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'c': // TON2
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          ton2[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'd': // TOFF2
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          toff2[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'e': // Duration
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          duration[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'f': // Delay Time
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          delayTime[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'g': // Polarity
          Serial.readBytes(serBuf, 2);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c", serBuf[1]);
          bitVal = atoi(cBuf);
          bitWrite(invOut, ch, bitVal);
          EEPROM.write(0, invOut);
          for(int ch = 0; ch < 8; ch++)
          {
            if (bitRead(invOut, ch) == 1)digitalWrite(out[ch], HIGH);
            else digitalWrite(out[ch], LOW);
          }
          break;

        case 's': // Start
          Serial.readBytes(&serBuf[0], 1);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          tcount[ch] = 0;
          pcount1[ch] = 0;
          pcount2[ch] = 0;
          lstate1[ch] = 1;
          lstate2[ch] = 1;
          run[ch] = 1;
          break;

        case 't': // Stop
          Serial.readBytes(&serBuf[0], 1);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          run[ch] = 0;
          if (bitRead(invOut, ch) == 1) digitalWrite(out[ch], HIGH);
          if (bitRead(invOut, ch) == 0) digitalWrite(out[ch], LOW);
          break;
      }
    }
  }
}

