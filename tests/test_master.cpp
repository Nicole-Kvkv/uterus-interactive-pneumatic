#include "Arduino.h"
#include "Wire.h"
#include <cassert>
#include <iostream>

void setup();
void loop();

void runFor(unsigned long durationMs) {
  const unsigned long end = millis() + durationMs;
  while (millis() < end) loop();
}
void verifyPacket() {
  assert(Wire.lastPacket.size() == 7);
  assert(Wire.lastPacket[0] == 0xA5);
  byte check = 0;
  for (int i = 0; i < 6; ++i) check ^= Wire.lastPacket[i];
  assert(Wire.lastPacket[6] == check);
}
void expectAllPumpsOff() {
  const byte en[] = {3, 5, 6, 9};
  for (byte pin : en) assert(mockPWM[pin] == 0);
  verifyPacket();
  assert(Wire.lastPacket[1] == 0);
}
int main() {
  setup();
  runFor(200);
  expectAllPumpsOff();

  // LEFT -> manual suction on all bags; releasing stops.
  mockInput[10] = 1;
  runFor(250);
  assert(mockPWM[9] == 255 && mockPWM[5] == 255);
  assert(mockPWM[6] == 0 && mockPWM[3] == 0);
  verifyPacket();
  assert(Wire.lastPacket[1] == 2);
  assert(Wire.lastPacket[3] == 255 && Wire.lastPacket[5] == 255);
  mockInput[10] = 0;
  runFor(250);
  expectAllPumpsOff();

  // RIGHT -> inflation; timeout at five seconds regardless of contact.
  mockInput[11] = 1;
  runFor(250);
  assert(mockPWM[6] == 255 && mockPWM[3] == 255);
  verifyPacket();
  assert(Wire.lastPacket[1] == 1);
  runFor(5100);
  expectAllPumpsOff();
  runFor(300);
  expectAllPumpsOff();
  mockInput[11] = 0;
  runFor(250);
  mockInput[11] = 1;
  runFor(250);
  assert(mockPWM[6] == 255);
  mockInput[11] = 0;
  runFor(250);
  expectAllPumpsOff();

  // Both hands -> 180s show when held for 2-5s and released.
  mockInput[10] = 1;
  mockInput[11] = 1;
  runFor(3000);
  mockInput[10] = 0;
  mockInput[11] = 0;
  runFor(250);
  assert(Serial.log.find("180S SHOW STARTED") != std::string::npos);
  assert(mockPWM[3] == 255); // Bag 2 continuously inflates in Stage I.
  verifyPacket();
  assert(Wire.lastPacket[1] == 3);

  // Both hands for >=6s stops running show.
  mockInput[10] = 1;
  mockInput[11] = 1;
  runFor(6300);
  expectAllPumpsOff();
  assert(Serial.log.find("BOTH 6S STOP") != std::string::npos);
  mockInput[10] = 0;
  mockInput[11] = 0;
  runFor(250);
  expectAllPumpsOff();

  // Serial R and S: serial stop requires release before new touch.
  Serial.pendingInput += "R";
  runFor(100);
  assert(Wire.lastPacket[1] == 3);
  Serial.pendingInput += "G";
  runFor(100);
  assert(Wire.lastPacket[1] == 4);
  Serial.pendingInput += "S";
  runFor(100);
  expectAllPumpsOff();

  std::cout << "Master mock tests: PASS\n";
  return 0;
}
