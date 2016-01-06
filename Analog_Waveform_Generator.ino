/* Analog Waveform Generator (Single Channel)*/
/* Designed by: ID&F, JRC-HHMI */
/* Author: Lakshminarayanan Ramasamy PhD */
/* Principal Investigator: Marta Zlatic  */
/* Date created: April 15th 2015*/
/* Date last modified: May 5th 2015 */

#include <SPI.h>
#include <MiniGen.h>
#include <TimerOne.h>


// Constant definition
#define lRate 1000 //Interrupt loop rate defined in microseconds
#define TPIN 8
#define nChannels 1

//User defined variables
unsigned long freqReg;
int nChar, cmdID, bID, run[8], tpSt, lstate[8], prvState, chan[8] = {10, 11, 12, 13, 14, 15, 16, 17};
double ton[8] , toff[8] , pcount[8], duration[8], tcount[8], delayTime[8], freq[8], wave[8];
char serBuf[7], cBuf[6];
MiniGen gen;


// toggleDebugPin function is called on every interrupt callback.
// USER can quickly debug whether the controller is working or NOT by probing TPIN for digital singal of frequecny=1/(2xlRate)
void toggleDebugPin(void)
{
  tpSt = !tpSt;
  digitalWrite(TPIN, tpSt);
}

/* Sets waveform frequency */
void setFrequency(double data, int ch)
{
MiniGen: MiniGen(chan[ch]);

  // freqCalc() makes a useful 32-bit value out of the frequency value (in Hz) passed to it.
  freqReg = gen.freqCalc(data);

  // Adjust the frequency. This is a full 32-bit write.
  gen.adjustFreq(MiniGen::FREQ0, freqReg);
}

/* Sets waveform type to sine, square or triangle */
void setWaveform(int wType, int ch)
{
MiniGen: MiniGen(chan[ch]);
  switch (wType)
  {
    case 0:
      gen.setMode(MiniGen::SINE);
      break;
    case 1:
      gen.setMode(MiniGen::TRIANGLE);
      break;
    case 2:
      gen.setMode(MiniGen::SQUARE);
      break;
  }
}


/* Timing control: Timer1 Interrupt is used for accurate timing control for waveform generation */
void intLoop(void)
{
  toggleDebugPin();
  for (int ch = 0; ch < nChannels; ch++)
  {
    int prvRun = run[ch];
    if (run[ch] == 1)
    {
      prvState = lstate[ch];
      tcount[ch] = tcount[ch] + lRate;
      if (tcount[ch] > (duration[ch] + delayTime[ch]))
      {
        run[ch] = 0;
        lstate[ch] = 0;
        pcount[ch] = 0;
        setFrequency(0, ch);
      }
      else
      {
        if (tcount[ch] > delayTime[ch])
        {
          pcount[ch] = pcount[ch] + lRate;
          if (lstate[ch] == 0)
          {
            if (pcount[ch] > toff[ch])
            {
              lstate[ch] = 1;
              pcount[ch] = 0;
            }
          }
          else
          {
            if (pcount[ch] > ton[ch])
            {
              lstate[ch] = 0;
              pcount[ch] = 0;
            }
          }
        }
      }
      if (prvState != lstate[ch])
      {
        if (lstate[ch] == 1) setFrequency(freq[ch], ch);
        if (lstate[ch] == 0) setFrequency(0, ch);
      }
      if (tcount[ch] - delayTime[ch] == lRate) setFrequency(freq[ch], ch);
    }
    else
    {
      if (prvRun != run[ch]) setFrequency(0, ch);
    }
  }
}

void setup()
{
  Serial.setTimeout(100);
  Serial.begin(115200);

  // Setting PIN direction to "OUTPUT"
  pinMode(TPIN, OUTPUT);
  pinMode(10, OUTPUT);

  gen.reset();
  gen.setMode(MiniGen::SINE); // Set waveform type to sine
  gen.setFreqAdjustMode(MiniGen::FULL); // Uses 32 bit register for setting the frequency

  // Default pulse settings
  for (int ch = 0; ch < 2; ch++)
  {
    run[ch] = 0;
    lstate[ch] = 0;
    setFrequency(0, ch);
    ton[ch] = 100000;
    toff[ch] = 100000;
    delayTime[ch] = 0;
    duration[ch] = 500000;
    freq[ch] = 1000;
  }
  /* Setup Timer 1 interrupt */
  Timer1.attachInterrupt(intLoop);
  Timer1.initialize(lRate); // Calls every "lRate" us
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
        case 'a': // TON
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          ton[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'b': // TOFF
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          toff[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'c': // Delay Time
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          delayTime[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'd': // Duration
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          duration[ch] = (double)atol(cBuf) * 1000;
          break;

        case 'e': // Frequency
          Serial.readBytes(serBuf, 7);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c%c%c%c%c%c", serBuf[1], serBuf[2], serBuf[3], serBuf[4], serBuf[5], serBuf[6]);
          freq[ch] = atol(cBuf);
          break;

        case 'f': // Waveform Type
          Serial.readBytes(serBuf, 2);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          sprintf(cBuf, "%c", serBuf[1]);
          wave[ch] = atoi(cBuf);
          setWaveform(wave[ch], ch);
          break;

        case 's': // Start
          Serial.readBytes(&serBuf[0], 1);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          tcount[ch] = 0;
          pcount[ch] = 0;
          lstate[ch] = 1;
          run[ch] = 1;
          break;

        case 't': // Stop
          Serial.readBytes(&serBuf[0], 1);
          sprintf(cBuf, "%c", serBuf[0]);
          ch = atoi(cBuf);
          run[ch] = 0;
          break;
      }
    }
  }
}

