#include <AD9850.h>
const int TX = 6;
const int W_CLK_PIN = 7;
const int FQ_UD_PIN = 8;
const int DATA_PIN = 9;
const int RESET_PIN = 10;
double freq=10000000;
double rxfreq = 51833333;
double txfreq = 8055000;
double trimFreq = 124999570;

int phase = 0;

void setup(){
  pinMode(TX, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  DDS.begin(W_CLK_PIN, FQ_UD_PIN, DATA_PIN, RESET_PIN);
  DDS.calibrate(trimFreq);
}

void loop(){
  freq=txfreq;
  DDS.setfreq(freq, phase);
  delay(10000);
  DDS.down();
  digitalWrite(LED_BUILTIN, LOW); 
  delay(3000);
  DDS.up();
  
  delay(2000);
  freq=rxfreq;
  DDS.setfreq(freq, phase);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(500000);
  DDS.down();
  while(1);
}