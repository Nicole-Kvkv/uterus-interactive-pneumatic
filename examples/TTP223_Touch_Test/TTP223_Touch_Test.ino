// SPDX-License-Identifier: MIT
// UTERUS — open-source pneumatic installation firmware
// See LICENSE and docs/SAFETY.md before operating physical pumps.

/*
  UTERUS - TTP223 TOUCH SENSOR TEST
  Arduino UNO Master; no I2C, pumps or Slave needed.
  LEFT TTP223 OUT -> D10
  RIGHT TTP223 OUT -> D11
  TTP223 modules: VCC -> regulated 5V (if compatible); GND -> common GND
  Assumes default HIGH when touched, LOW when released.
  Debounce: 80 ms press; 150 ms release.
*/

const byte LEFT_PIN = 10;
const byte RIGHT_PIN = 11;
const unsigned long PRESS_MS = 80UL;
const unsigned long RELEASE_MS = 150UL;

struct SensorState {
  bool stable;
  bool candidate;
  unsigned long changedAt;
};

SensorState leftSensor = {false, false, 0};
SensorState rightSensor = {false, false, 0};
byte lastState = 255;
unsigned long lastPrintAt = 0;

void updateSensor(SensorState &sensor, bool reading, unsigned long now) {
  if (reading != sensor.candidate) {
    sensor.candidate = reading;
    sensor.changedAt = now;
  }
  unsigned long waitMs = sensor.candidate ? PRESS_MS : RELEASE_MS;
  if (sensor.stable != sensor.candidate &&
      now - sensor.changedAt >= waitMs) {
    sensor.stable = sensor.candidate;
  }
}

const char* stateName(byte state) {
  switch (state) {
    case 1: return "LEFT";
    case 2: return "RIGHT";
    case 3: return "BOTH";
    default: return "NONE";
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LEFT_PIN, INPUT);
  pinMode(RIGHT_PIN, INPUT);
  Serial.println(F("UTERUS - TTP223 TOUCH TEST READY"));
  Serial.println(F("LEFT=D10, RIGHT=D11, HIGH=TOUCH"));
  Serial.println(F("TOUCH STATES: NONE / LEFT / RIGHT / BOTH"));
}

void loop() {
  unsigned long now = millis();
  bool leftRaw = (digitalRead(LEFT_PIN) == HIGH);
  bool rightRaw = (digitalRead(RIGHT_PIN) == HIGH);
  updateSensor(leftSensor, leftRaw, now);
  updateSensor(rightSensor, rightRaw, now);

  byte state = (leftSensor.stable ? 1 : 0) +
               (rightSensor.stable ? 2 : 0);
  if (state != lastState) {
    Serial.print(F(">>> STATE: "));
    Serial.println(stateName(state));
    lastState = state;
  }

  if (now - lastPrintAt >= 200UL) {
    lastPrintAt = now;
    Serial.print(F("LEFT="));
    Serial.print(leftRaw ? 1 : 0);
    Serial.print(F(" | RIGHT="));
    Serial.print(rightRaw ? 1 : 0);
    Serial.print(F(" | STATE="));
    Serial.println(stateName(state));
  }
}
