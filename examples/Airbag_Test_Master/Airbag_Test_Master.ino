// SPDX-License-Identifier: MIT
// UTERUS — open-source pneumatic installation firmware
// See LICENSE and docs/SAFETY.md before operating physical pumps.

#include <Wire.h>

/*
  UTERUS - AIRBAG FILL/DRAIN TEST - MASTER
  Arduino UNO Master (Bag 1 + Bag 2 locally, Bag 3 + Bag 4 via Slave)
  Does not read TTP223 and does not run the 180-second show.

  Serial at 9600 baud:
    1  Bag 1 FILL   5  Bag 1 DRAIN
    2  Bag 2 FILL   6  Bag 2 DRAIN
    3  Bag 3 FILL   7  Bag 3 DRAIN
    4  Bag 4 FILL   8  Bag 4 DRAIN
    S  STOP ALL

  Each command drives ONE pump at a time for 1000 milliseconds.
  Bag 3 and Bag 4 require the Slave V17.2 receiver on address 0x08.
*/

const byte SLAVE_ADDR = 0x08;
const unsigned long TEST_MS = 1000UL;
const unsigned long HEARTBEAT_MS = 50UL;

// Bag 1 (Master)
const byte B1_FE = 6, B1_F1 = 12, B1_F2 = A0;
const byte B1_DE = 9, B1_D1 = A1, B1_D2 = A2;

// Bag 2 (Master)
const byte B2_FE = 3, B2_F1 = 2, B2_F2 = 4;
const byte B2_DE = 5, B2_D1 = 7, B2_D2 = 8;

// Index 0..3 = Bags 1..4
byte fillPWM[4] = {0, 0, 0, 0};
byte drainPWM[4] = {0, 0, 0, 0};

bool testActive = false;
unsigned long startedAt = 0;
unsigned long lastHeartbeatAt = 0;
bool slavePresent = false;

void pump(byte en, byte in1, byte in2, byte pwm) {
  analogWrite(en, 0);
  if (pwm == 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(en, pwm);
  }
}

void applyMaster() {
  pump(B1_FE, B1_F1, B1_F2, fillPWM[0]);
  pump(B1_DE, B1_D1, B1_D2, drainPWM[0]);
  pump(B2_FE, B2_F1, B2_F2, fillPWM[1]);
  pump(B2_DE, B2_D1, B2_D2, drainPWM[1]);
}

void sendSlave() {
  // 7-byte packet compatible with Slave V17.2:
  // 0xA5, mode, B3fill, B3drain, B4fill, B4drain, XOR
  byte packet[7];
  packet[0] = 0xA5;
  packet[1] = 0; // STOP unless a remote pump is on
  if (fillPWM[2] || fillPWM[3]) packet[1] = 1;
  if (drainPWM[2] || drainPWM[3]) packet[1] = 2;
  packet[2] = fillPWM[2];
  packet[3] = drainPWM[2];
  packet[4] = fillPWM[3];
  packet[5] = drainPWM[3];
  byte checksum = 0;
  for (byte i = 0; i < 6; i++) checksum ^= packet[i];
  packet[6] = checksum;

  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(packet, 7);
  byte result = Wire.endTransmission();
  if (result == 0) slavePresent = true;
}

void allOff() {
  for (byte i = 0; i < 4; i++) {
    fillPWM[i] = 0;
    drainPWM[i] = 0;
  }
  testActive = false;
  applyMaster();
  sendSlave();
}

void startOnePump(byte command) {
  // Ensure previous pump is OFF before starting a new one.
  allOff();
  delay(100); // allow pump/driver to settle between commands

  byte bag = (command <= 4) ? command : command - 4;
  bool fill = (command <= 4);
  if (bag >= 3 && !slavePresent) {
    // Recheck slave presence for remote-bag tests.
    Wire.beginTransmission(SLAVE_ADDR);
    slavePresent = (Wire.endTransmission() == 0);
    if (!slavePresent) {
      Serial.println(F("SLAVE NOT FOUND: cannot test BAG 3/4"));
      return;
    }
  }

  byte index = bag - 1;
  if (fill) fillPWM[index] = 255;
  else drainPWM[index] = 255;

  applyMaster();
  sendSlave();
  lastHeartbeatAt = millis();
  startedAt = millis();
  testActive = true;

  Serial.print(F("BAG "));
  Serial.print(bag);
  Serial.println(fill ? F(" FILL 1S") : F(" DRAIN 1S"));
}

void setup() {
  Serial.begin(9600);
  const byte pins[12] = {
    B1_FE, B1_F1, B1_F2,
    B1_DE, B1_D1, B1_D2,
    B2_FE, B2_F1, B2_F2,
    B2_DE, B2_D1, B2_D2
  };
  for (byte i = 0; i < 12; i++) {
    digitalWrite(pins[i], LOW);
    pinMode(pins[i], OUTPUT);
  }
  applyMaster();

  Wire.begin();
  Wire.setWireTimeout(25000, true); // UNO AVR I2C timeout
  delay(100);
  Wire.beginTransmission(SLAVE_ADDR);
  slavePresent = (Wire.endTransmission() == 0);
  allOff();

  Serial.println(F("UTERUS - INDIVIDUAL AIRBAG TEST READY"));
  Serial.println(F("1-4 = BAG 1-4 FILL (1s)"));
  Serial.println(F("5-8 = BAG 1-4 DRAIN (1s)"));
  Serial.println(F("S = STOP ALL"));
  Serial.println(slavePresent ? F("SLAVE FOUND: 0x08") : F("SLAVE NOT FOUND"));
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= '1' && c <= '8') startOnePump((byte)(c - '0'));
    else if (c == 's' || c == 'S') {
      allOff();
      Serial.println(F("ALL STOP"));
    }
  }

  unsigned long now = millis();
  if (testActive && now - startedAt >= TEST_MS) {
    allOff();
    Serial.println(F("AUTO STOP AFTER 1S"));
  }

  if (now - lastHeartbeatAt >= HEARTBEAT_MS) {
    lastHeartbeatAt = now;
    sendSlave(); // keeps Slave watchdog alive during remote tests
  }
}
