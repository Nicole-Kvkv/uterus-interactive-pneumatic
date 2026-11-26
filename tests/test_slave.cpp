#include "Arduino.h"
#include "Wire.h"
#include <cassert>
#include <iostream>
#include <vector>

void setup();
void loop();

std::vector<byte> frame(byte mode, byte b3fill, byte b3drain,
                        byte b4fill, byte b4drain) {
  std::vector<byte> packet = {0xA5,mode,b3fill,b3drain,b4fill,b4drain,0};
  for (int i = 0; i < 6; ++i) packet[6] ^= packet[i];
  return packet;
}
int main() {
  setup();
  assert(Wire.slaveAddress == 0x08);
  assert(mockPWM[3] == 0 && mockPWM[5] == 0);

  // Valid inflation command controls Bag 3 and Bag 4.
  Wire.injectReceive(frame(1,255,0,255,0));
  loop();
  assert(mockPWM[3] == 255 && mockPWM[6] == 255);
  assert(mockPWM[5] == 0 && mockPWM[9] == 0);

  // Invalid checksum is discarded; eventually the watchdog stops pumps.
  auto bad = frame(1,255,0,255,0);
  bad[6] ^= 0x42;
  Wire.injectReceive(bad);
  delay(450);
  loop();
  assert(mockPWM[3] == 0 && mockPWM[6] == 0);

  // Mutual exclusion on one bag.
  Wire.injectReceive(frame(3,255,255,0,255));
  loop();
  assert(mockPWM[3] == 0 && mockPWM[5] == 0);
  assert(mockPWM[9] == 255);

  // STOP overrides all PWM fields.
  Wire.injectReceive(frame(0,255,255,255,255));
  loop();
  assert(mockPWM[3] == 0 && mockPWM[5] == 0 &&
         mockPWM[6] == 0 && mockPWM[9] == 0);

  std::cout << "Slave mock tests: PASS\n";
  return 0;
}
