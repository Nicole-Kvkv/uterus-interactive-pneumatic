#include "Arduino.h"
#include "Wire.h"
unsigned long mockMillis = 0;
int mockInput[32] = {};
int mockOutput[32] = {};
int mockPWM[32] = {};
bool mockInterruptsEnabled = true;
MockSerial Serial;
MockWire Wire;
