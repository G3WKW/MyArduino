/*   A simple sketch to control an ADF4351 module from a PC via an 
     Arduino Uno or equivalent, using the Arduino IDE's Serial Monitor.
     Adapted from a sketch by Alain Fort F1CJN
     By Jim Rowe, for Silicon Chip
     Last revised February 20, 2018 at 10:30 am
  
     Assumed connections between the Arduino and the ADF4351 module: 
     SPI line     Pin on Arduino    Pin on ADF4351 module
     MOSI         Pin 4 of ICSP     DAT (pin 5)
     SCK          Pin 2 of ICSP     CLK (pin 4)
     LE           DIO pin 3         LE  (pin 6)
     (Note that all three of the above connections from the Arduino to the
     ADF4351 module should be made via voltage dividers using 1.5kohm upper
     resistors and 3.0kohm lower resistors, to lower the logic high voltage
     levels from 5V to 3.3V. That is because the ADF4351 chip operates from a
     3.3V supply, and can be damaged if its inputs are taken above 3.6V.)///////////////////////////////////////////////////////////
     The remaining connections are from GND on the Arduino (pin 6 of the ICSP
     connector) and pins 7 and 10 on the ADF4351 module (both ground), and
     from the lock detect pin of the ADF4351 module (pin 2) to DIO pin 2 
     on the Arduino. This connection needs no voltage divider because it is 
     used to take LOCK STATUS data back from the ADF4351 to the Arduino.
     The ADF4351 module can be powered from a 5V 'USB plug pack' or directly
     from the +5V and GND pins of the Arduino.*/

#include <SPI.h>
#include <Wire.h>

#define ADF4351_LE 3  // define ADF4351_LE pin as DIO pin 3

unsigned long ADFReg[6];   // declare an array of six 32-bit unsigned ints
// then define some variables
double RFout, RFoutNew;   // current & new values for RFout
double PFDRFout = 25;   // value for a 25MHz REF, with D & T = 0, R = 1
double FRACF;           // FP equivalent of FRAC, used in calculations
double OutChanSpacing = 0.01;   // minimum frequency step (10kHz)
double RFoutMin = 35, RFoutMax = 4400;  // limiting values for RFout
unsigned long RFint, RFintNew, INTA, MOD, FRAC;
byte OutputDivider; byte Lock = 0;  // Lock = 1 when PLL is locked
unsigned long Val32b;   // variable used to pass 32-bit data

void setup()
{
  // give initial values for the six ADF4351 control registers
  ADFReg[0] = 0x4580A8;   // Reg 0: set INTA = 139d, FRAC = 21d
  ADFReg[1] = 0x80080C9;  // Reg 1: MOD = 25d, DBR = 1, PreS = 8/9
  ADFReg[2] = 0x4E42;     // Reg 2: Rctr = 1, LDF = 2.5mA, LDP = 10ns
  ADFReg[3] = 0x4B3;      // Reg 3: Clock Divider = 150d, but OFF
  ADFReg[4] = 0xBC803C;   // Reg 4: O/P pwr=+5dBm,VCO on,bandsel=200,RFdiv=8
  ADFReg[5] = 0x580005;   // Reg 5: LD pin set for dig lock detect
   
  Serial.begin (9600); // open serial to the IDE 'Serial Monitor' at 9600 baud
  Serial.println("ADF4351 module checkout sketch");
  pinMode(2, INPUT);                // Pin 2 is the input for the LD signal
  pinMode(ADF4351_LE, OUTPUT);      // pin 3 (ADF4351_LE pin) is an output
  digitalWrite(ADF4351_LE, HIGH);   // and set it initially to high
  SPI.begin();                      // Initialise the SPI bus
  SPI.setDataMode(SPI_MODE0);       // CPHA = 0 for clock rising edge
  SPI.setBitOrder(MSBFIRST);        // and data will be sent MSB first
  Serial.println("Now initialising the ADF4351");
  SetADF4351();                      // go initialise the six ADF4351 ADFReg
  delay(100);       // brief pause so PLL has time to lock 
  RFint = 43700;              // initial value of RFint ( = 437.00MHz)
  RFout = float(RFint/100);          // the corresponding output frequency in MHz
} // end of setup

void loop()
{
  if (digitalRead(2) == HIGH)   // check PLL lock status
  {
    Lock = 1;                 // set Lock = 1 if it's locked
  }
  else
  {
    Lock = 0;                 // otherwise reset to zero
  }
  Serial.print("Current Frequency = ");   // now print current status
  Serial.print(RFout);
  Serial.print(" MHz ");
  Serial.print("Lock = ");
  Serial.println(Lock);
  // then invite user to key in new frequency:
  Serial.println("To change frequency, type in new value in MHz (NNNN.NN):");
  while (Serial.available() <= 1)  // wait until something arrives from the PC,
  {
    delay(1000);
  }
  // once something has arrived, read it & process
  {
    RFoutNew = Serial.parseFloat();     // read it into RFoutNew
    RFintNew = long(RFoutNew * 100);    // find equiv value for RFintNew
    if (RFintNew == RFint)        // but if it's the same as current,
    {
      Serial.println("No change?");  // just query it
    }
    else
    {
      if ((RFoutNew < RFoutMin) || (RFoutNew > RFoutMax))
      {
        Serial.println("Sorry, outside the range of the ADF4351!");
      }
      else
      {
        UpdateRegs();        // otherwise go work out the new Reg settings
        RFint = RFintNew;    // make RFint = RFintNew
        SetADF4351();        // then reprogram the ADF4351 regs     
      }
    }
    delay(100);       // pause for 100ms for ADF4351 to lock
  }
}   // end of main loop
//***************************************************************************
// subroutine to update the ADF4351 registers for a new output frequency
void UpdateRegs()
  {
    RFout = RFoutNew;             // OK to make RFoutNew the current RFout
    if (RFout >= 2200)            // now find the right RF div setting
    {
      OutputDivider = 1;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 0);  // & set RF divider for 1
      bitWrite (ADFReg[4], 21, 0);
      bitWrite (ADFReg[4], 20, 0);
    }
    if ((RFout < 2200) && (RFout >= 1100))
    {
      OutputDivider = 2;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental      
      bitWrite (ADFReg[4], 22, 0);  // & set RF divider for 2
      bitWrite (ADFReg[4], 21, 0);
      bitWrite (ADFReg[4], 20, 1);
    }
    if ((RFout < 1100) && (RFout >= 550))
    {
      OutputDivider = 4;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 0);  // & set RF divider for 4
      bitWrite (ADFReg[4], 21, 1);
      bitWrite (ADFReg[4], 20, 0);
    }
    if ((RFout < 550) && (RFout >= 275))
    {
      OutputDivider = 8;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 0);  // & set RF divider for 8
      bitWrite (ADFReg[4], 21, 1);
      bitWrite (ADFReg[4], 20, 1);
    }
    if ((RFout < 275) && (RFout >= 137.5))
    {
      OutputDivider = 16;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 1);  // & set RF divider for 16
      bitWrite (ADFReg[4], 21, 0);
      bitWrite (ADFReg[4], 20, 0);
    }
    if ((RFout < 137.5) && (RFout >= 68.75))
    {
      OutputDivider = 32;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 1);  // & set RF divider for 32
      bitWrite (ADFReg[4], 21, 0);
      bitWrite (ADFReg[4], 20, 1);
    }
    if ((RFout < 68.75) && (RFout >= 35))
    {
      OutputDivider = 64;
      bitWrite (ADFReg[4], 23, 1);  // feedback from VCO fundamental
      bitWrite (ADFReg[4], 22, 1);  // & set RF divider for 64
      bitWrite (ADFReg[4], 21, 1);
      bitWrite (ADFReg[4], 20, 0);
    }
    // work out new values for INTA, MOD and FRAC:
    INTA = (RFout * OutputDivider) / PFDRFout;
    MOD = (PFDRFout / OutChanSpacing);
    FRACF = (((RFout * OutputDivider) / PFDRFout) - INTA) * MOD;
    FRAC = round(FRACF);  // round FRACF (FP) to get FRAC as an integer
    
    // now we can set up ADFReg[0]:
    ADFReg[0] = 0;                // first clear it
    ADFReg[0] = INTA << 15;       // then slot in INTA to bits 15-30
    FRAC = FRAC << 3;             // also bump up FRAC by 3 bits
    ADFReg[0] = ADFReg[0] + FRAC; // and add it in to complete this reg
    
    // and ADFReg[1]:
    ADFReg[1] = 0;                // clear it first
    ADFReg[1] = MOD << 3;   // add MOD, shifted up by 3 bits to clear reg addr
    ADFReg[1] = ADFReg[1] + 1 ;   // add in '001' for correct reg addr
    bitSet(ADFReg[1], 15);        // set DB15 for min phase value of 1
    bitSet(ADFReg[1], 27);       // also set prescaler bit (27) for 8/9
    
    // and ADFReg[2]:
    bitClear(ADFReg[2], 28);   // 000 = MUX OUT tristated (disabled)
    bitClear(ADFReg[2], 27); 
    bitClear(ADFReg[2], 26);

    // and ADFReg[3]:
    ADFReg[3] = 0x4B3;      // make sure settings are unchanged

    // and ADFReg[4}:
    bitClear (ADFReg[4], 0);  // make sure reg address is OK
    bitClear (ADFReg[4], 1);
    bitSet(ADFReg[4], 2);
    bitSet(ADFReg[4], 3);    // and RF O/P pwr is set for +5dBm
    bitSet(ADFReg[4], 4);
    bitSet(ADFReg[4], 5);   // and RF O/P is enabled

    // and finally ADFReg[5]:
    ADFReg[5] = 0x580005;   // LD pin mode set for dig lock detect
    
  } // end of UpdateRegs() sub
// *************************************************************************
// subroutine/function to program a 32-bit ADF4351 register with 'Val32b'
void WriteRegister32()
{
  digitalWrite(ADF4351_LE, LOW);
  for (int i = 3; i >= 0; i--)  // loop to send value in 4 x 8-bit chunks
  {
    SPI.transfer((Val32b >> (8 * i)) & 0xFF); // move down & send only LS byte
  }
    digitalWrite(ADF4351_LE, HIGH); // after all 4 bytes sent, pulse LE line
    digitalWrite(ADF4351_LE, LOW);  // high briefly to latch into Register
}
// **************************************************************************
// subroutine to program all six of the ADF4351 control ADFRegs with the
// values in ADFReg[5, 4, 3, 2, 1, 0] (decreasing order as recommended)
void SetADF4351()
{
  for (int x = 5; x >= 0; x--)
  {
    Serial.print("Reg[");         // for testing
    Serial.print(x);
    Serial.print("] ");
    Val32b = ADFReg[x];
    Serial.println(Val32b, HEX);       // also for testing
    WriteRegister32();
  }
}
// **************************************************************************
