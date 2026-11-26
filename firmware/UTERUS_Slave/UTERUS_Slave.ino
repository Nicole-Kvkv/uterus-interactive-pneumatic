// SPDX-License-Identifier: MIT
// UTERUS — open-source pneumatic installation firmware
// See LICENSE and docs/SAFETY.md before operating physical pumps.

#include <Wire.h>

/*
  UTERUS — SLAVE V17.2
  Board: Arduino UNO
  Controls Bag 3 and Bag 4, accepts I2C packets at address 0x08.
  Compatible with UTERUS Master V17.4.
  Receive format: [0xA5, mode, B3Fill, B3Drain, B4Fill, B4Drain, XOR].
  PWM output: 0=off, 255=full power; the master determines choreography.
  Watchdog: stop pumps after 400 ms without a valid packet.
*/

const byte SLAVE_ADDRESS = 0x08;
const byte PACKET_HEADER = 0xA5;
const unsigned long WATCHDOG_MS = 400UL;

// Slave Bag 3 (same pin layout as Master's Bag 2).
const byte B3_FILL_EN = 3;
const byte B3_FILL_IN1 = 2;
const byte B3_FILL_IN2 = 4;
const byte B3_DRAIN_EN = 5;
const byte B3_DRAIN_IN1 = 7;
const byte B3_DRAIN_IN2 = 8;

// Slave Bag 4 (same pin layout as Master's Bag 1).
const byte B4_FILL_EN = 6;
const byte B4_FILL_IN1 = 12;
const byte B4_FILL_IN2 = A0;
const byte B4_DRAIN_EN = 9;
const byte B4_DRAIN_IN1 = A1;
const byte B4_DRAIN_IN2 = A2;

volatile byte receivedPacket[7] = {0};
volatile bool receivedReady = false;

byte pumpPWM[4] = {0, 0, 0, 0};
byte activeMode = 0;
unsigned long lastValidPacketAt = 0;

void motor(byte en, byte in1, byte in2, byte pwm) {
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

void applyOutputs() {
  // Interlock: do not inflate and drain the same bag concurrently.
  if (pumpPWM[0] && pumpPWM[1]) {
    pumpPWM[0] = 0;
    pumpPWM[1] = 0;
  }
  if (pumpPWM[2] && pumpPWM[3]) {
    pumpPWM[2] = 0;
    pumpPWM[3] = 0;
  }

  motor(B3_FILL_EN, B3_FILL_IN1, B3_FILL_IN2, pumpPWM[0]);
  motor(B3_DRAIN_EN, B3_DRAIN_IN1, B3_DRAIN_IN2, pumpPWM[1]);
  motor(B4_FILL_EN, B4_FILL_IN1, B4_FILL_IN2, pumpPWM[2]);
  motor(B4_DRAIN_EN, B4_DRAIN_IN1, B4_DRAIN_IN2, pumpPWM[3]);
}

void stopOutputs() {
  for (byte i = 0; i < 4; i++) pumpPWM[i] = 0;
  activeMode = 0;
  applyOutputs();
}

// Called by Wire library inside the I2C receive interrupt.
// Keep ISR short; never use Serial printing here.
void receiveEvent(int numberOfBytes) {
  (void)numberOfBytes;
  byte data[7];
  byte count = 0;
  while (Wire.available()) {
    byte value = Wire.read();
    if (count < 7) data[count] = value;
    count++;
  }
  if (count != 7) return;
  if (data[0] != PACKET_HEADER || data[1] > 4) return;

  byte checksum = 0;
  for (byte i = 0; i < 6; i++) checksum ^= data[i];
  if (checksum != data[6]) return;

  for (byte i = 0; i < 7; i++) receivedPacket[i] = data[i];
  receivedReady = true;
}

void setup() {
  Serial.begin(9600);
  const byte pins[12] = {
    B3_FILL_EN, B3_FILL_IN1, B3_FILL_IN2,
    B3_DRAIN_EN, B3_DRAIN_IN1, B3_DRAIN_IN2,
    B4_FILL_EN, B4_FILL_IN1, B4_FILL_IN2,
    B4_DRAIN_EN, B4_DRAIN_IN1, B4_DRAIN_IN2
  };
  for (byte i = 0; i < 12; i++) {
    digitalWrite(pins[i], LOW);
    pinMode(pins[i], OUTPUT);
  }
  stopOutputs();

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  lastValidPacketAt = millis();

  Serial.println(F("UTERUS SLAVE V17.2 READY"));
  Serial.println(F("I2C 0x08; BAG 3 + BAG 4; 400MS WATCHDOG"));
}

void loop() {
  byte packet[7];
  bool hasPacket = false;

  noInterrupts();
  if (receivedReady) {
    for (byte i = 0; i < 7; i++) packet[i] = receivedPacket[i];
    receivedReady = false;
    hasPacket = true;
  }
  interrupts();

  if (hasPacket) {
    lastValidPacketAt = millis();
    activeMode = packet[1];

    if (activeMode == 0) {
      stopOutputs();
    } else {
      pumpPWM[0] = packet[2];  // Bag 3 fill
      pumpPWM[1] = packet[3];  // Bag 3 drain
      pumpPWM[2] = packet[4];  // Bag 4 fill
      pumpPWM[3] = packet[5];  // Bag 4 drain
      applyOutputs();
    }
  }

  if (activeMode != 0 && millis() - lastValidPacketAt > WATCHDOG_MS) {
    stopOutputs();
    Serial.println(F("I2C TIMEOUT: SLAVE PUMPS STOPPED"));
  }
}
