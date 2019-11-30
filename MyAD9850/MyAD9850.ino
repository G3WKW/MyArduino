#include <AD9850.h>
const int TX = 6;
const int W_CLK_PIN = 7;
const int FQ_UD_PIN = 8;
const int DATA_PIN = 9;
const int RESET_PIN = 10;
double freq=10000000;
double rxfreq = 144.950;
double txfreq = 144.950;
double trimFreq = 124999570;

int phase = 0;

void setup(){
  pinMode(TX, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  DDS.begin(W_CLK_PIN, FQ_UD_PIN, DATA_PIN, RESET_PIN);
  DDS.calibrate(trimFreq);
   DDS.up();
}

void loop(){
  freq=(rxfreq - 10.7)*1000000/12;
  DDS.setfreq(freq, phase);
  digitalWrite(LED_BUILTIN, LOW); 
  while(!digitalRead(TX));
 
  freq=txfreq*1000000/16;
  DDS.setfreq(freq, phase);
  digitalWrite(LED_BUILTIN, HIGH); 
  while(digitalRead(TX));
  
  
 
}
