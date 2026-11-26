// SPDX-License-Identifier: MIT
// UTERUS — open-source pneumatic installation firmware
// See LICENSE and docs/SAFETY.md before operating physical pumps.

#include <Wire.h>

/*
  UTERUS — MASTER V17.4
  Board: Arduino UNO (5 V)
  Touch sensors: two TTP223, active HIGH
     LEFT  OUT -> D10  (hold = deflate, max 5 seconds)
     RIGHT OUT -> D11  (hold = inflate, max 5 seconds)
     BOTH 2-5 seconds then release -> 180-second show
     BOTH held 6 seconds -> software stop
  Slave: UNO, I2C address 0x08, 7-byte XOR packet (Slave V17.2)

  IMPORTANT: Power pumps from a suitable external supply. Provide
  a physical emergency power cutoff and a pressure-relief path.
  Time limits are NOT pressure limits.
*/

// ----------------------- TOUCH -----------------------
const byte LEFT_PIN = 10;
const byte RIGHT_PIN = 11;
const unsigned long PRESS_DEBOUNCE_MS = 80UL;
const unsigned long RELEASE_DEBOUNCE_MS = 150UL;
const unsigned long MANUAL_MAX_MS = 5000UL;
const unsigned long BOTH_SHOW_MIN_MS = 2000UL;
const unsigned long BOTH_SHOW_MAX_MS = 5000UL;
const unsigned long BOTH_STOP_MS = 6000UL;
const unsigned long SHOW_TOTAL_MS = 180000UL;

// Channel 0 = LEFT, channel 1 = RIGHT.
bool touchStable[2] = {false, false};
bool touchCandidate[2] = {false, false};
unsigned long touchChangedAt[2] = {0, 0};

void debounceTouch(byte channel, bool reading, unsigned long now) {
  if (reading != touchCandidate[channel]) {
    touchCandidate[channel] = reading;
    touchChangedAt[channel] = now;
  }

  unsigned long waitMs = touchCandidate[channel]
    ? PRESS_DEBOUNCE_MS : RELEASE_DEBOUNCE_MS;

  if (touchStable[channel] != touchCandidate[channel] &&
      now - touchChangedAt[channel] >= waitMs) {
    touchStable[channel] = touchCandidate[channel];
  }
}

// ----------------------- PINS -----------------------
// Master Bag 1: inflate pump, then suction pump.
const byte B1_FILL_EN = 6;
const byte B1_FILL_IN1 = 12;
const byte B1_FILL_IN2 = A0;
const byte B1_DRAIN_EN = 9;
const byte B1_DRAIN_IN1 = A1;
const byte B1_DRAIN_IN2 = A2;

// Master Bag 2: inflate pump, then suction pump.
const byte B2_FILL_EN = 3;
const byte B2_FILL_IN1 = 2;
const byte B2_FILL_IN2 = 4;
const byte B2_DRAIN_EN = 5;
const byte B2_DRAIN_IN1 = 7;
const byte B2_DRAIN_IN2 = 8;

// ----------------------- STATES -----------------------
const byte SLAVE_ADDRESS = 0x08;
const byte PACKET_HEADER = 0xA5;
const byte MODE_STOP = 0;
const byte MODE_FILL = 1;
const byte MODE_DRAIN = 2;
const byte MODE_GROW = 3;
const byte MODE_HOLD = 4;

const byte RUN_IDLE = 0;
const byte RUN_FILL = 1;
const byte RUN_DRAIN = 2;
const byte RUN_SHOW = 3;
byte runState = RUN_IDLE;

unsigned long manualStartedAt = 0;
unsigned long showStartedAt = 0;
bool growthEnabled = true;

// Indexed as bag 0..3 for physical Bags 1..4.
byte bagFill[4] = {0, 0, 0, 0};
byte bagDrain[4] = {0, 0, 0, 0};

void clearBags() {
  for (byte i = 0; i < 4; i++) {
    bagFill[i] = 0;
    bagDrain[i] = 0;
  }
}

void setBag(byte bag, bool fill, bool drain) {
  if (bag >= 4) return;
  // Opposite pumps of one bag never run at the same time.
  if (fill && drain) { fill = false; drain = false; }
  bagFill[bag] = fill ? 255 : 0;
  bagDrain[bag] = drain ? 255 : 0;
}

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

void applyMaster() {
  motor(B1_FILL_EN, B1_FILL_IN1, B1_FILL_IN2, bagFill[0]);
  motor(B1_DRAIN_EN, B1_DRAIN_IN1, B1_DRAIN_IN2, bagDrain[0]);
  motor(B2_FILL_EN, B2_FILL_IN1, B2_FILL_IN2, bagFill[1]);
  motor(B2_DRAIN_EN, B2_DRAIN_IN1, B2_DRAIN_IN2, bagDrain[1]);
}

// Packet: [0xA5, mode, Bag3Fill, Bag3Drain, Bag4Fill, Bag4Drain, XOR].
void sendSlave(byte mode) {
  byte packet[7];
  packet[0] = PACKET_HEADER;
  packet[1] = mode;
  packet[2] = bagFill[2];
  packet[3] = bagDrain[2];
  packet[4] = bagFill[3];
  packet[5] = bagDrain[3];
  byte checksum = 0;
  for (byte i = 0; i < 6; i++) checksum ^= packet[i];
  packet[6] = checksum;

  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(packet, 7);
  Wire.endTransmission();
}

void stopAll() {
  // Stop locally first, even if I2C is disconnected.
  runState = RUN_IDLE;
  clearBags();
  applyMaster();
  sendSlave(MODE_STOP);
  Serial.println(F("ALL PUMPS STOPPED"));
}

void startFill() {
  runState = RUN_FILL;
  manualStartedAt = millis();
  Serial.println(F("RIGHT: FILL START (MAX 5S)"));
}

void startDrain() {
  runState = RUN_DRAIN;
  manualStartedAt = millis();
  Serial.println(F("LEFT: DRAIN START (MAX 5S)"));
}

void stopManual() {
  if (runState == RUN_FILL || runState == RUN_DRAIN) {
    stopAll();
  }
}

void startShow() {
  runState = RUN_SHOW;
  showStartedAt = millis();
  growthEnabled = true;
  Serial.println(F("180S SHOW STARTED"));
}

void disableGrowth() {
  if (runState == RUN_SHOW) {
    growthEnabled = false;
    Serial.println(F("GROWTH BIAS DISABLED"));
  }
}

// ----------------------- BREATHING -----------------------
// One 5-second cycle; 100-ms interval between fill and drain.
void breathing(byte bag, unsigned long t, unsigned long offset,
               unsigned long fillMs, unsigned long drainMs) {
  const unsigned long CYCLE = 5000UL;
  unsigned long phase = (t + offset) % CYCLE;

  if (phase < fillMs) {
    setBag(bag, true, false);
  } else if (phase >= fillMs + 100UL &&
             phase < fillMs + 100UL + drainMs) {
    setBag(bag, false, true);
  } else {
    setBag(bag, false, false);
  }
}

// Five stages exactly as in the established V17.4 choreography.
// Units are milliseconds; output is pump actuation, NOT pressure or volume.
void updateShow(unsigned long elapsed) {
  clearBags();

  if (elapsed < 5000UL) {
    // I. Fetal Breath Initiation — 0-5s
    setBag(1, true, false);              // Bag 2 inflates for all first 5s.
    breathing(0, elapsed, 0,    1800, 1800);
    breathing(2, elapsed, 1500, 1600, 1800);
    breathing(3, elapsed, 3000, 1800, 1800);

  } else if (elapsed < 30000UL) {
    // II. Uterine Expansion — 5-30s
    unsigned long t = elapsed - 5000UL;
    breathing(0, t, 0,    2600, 1400);
    breathing(1, t, 1200, 3000, 1000);
    breathing(2, t, 2500, 2200, 1700);
    breathing(3, t, 3700, 2800, 1100);

  } else if (elapsed < 60000UL) {
    // III. Uterine Wall Response — 30-60s
    unsigned long t = elapsed - 30000UL;
    breathing(0, t, 0,    2100, 1900);
    breathing(1, t, 2500, 2600, 1400);
    breathing(2, t, 1200, 1800, 2200);
    breathing(3, t, 3700, 2600, 1400);

  } else if (elapsed < 170000UL) {
    // IV. Fetal Breath Resonance — 60-170s
    unsigned long t = elapsed - 60000UL;
    breathing(0, t, 0,    2200, 1700);
    breathing(1, t, 1250, 2400, 1500);
    breathing(2, t, 2500, 1900, 2000);
    breathing(3, t, 3750, 2300, 1600);

  } else {
    // V. Return to Stillness — 170-180s
    unsigned long t = elapsed - 170000UL;
    if (t < 5000UL) {
      breathing(0, t, 0,    1700, 1700);
      breathing(1, t, 1250, 1800, 1600);
      breathing(2, t, 2500, 1500, 1800);
      breathing(3, t, 3750, 1700, 1700);
    } else {
      breathing(0, t, 0,    1000, 1000);
      breathing(1, t, 1250, 1200, 1000);
      breathing(2, t, 2500, 900,  1100);
      breathing(3, t, 3750, 1100, 1000);
    }
  }

  // Growth bias during Stage II: extra inflation of Bags 2 and 4.
  if (growthEnabled && elapsed >= 5000UL && elapsed < 30000UL) {
    unsigned long phase = (elapsed - 5000UL) % 5000UL;
    if (phase >= 2400UL && phase < 3000UL) {
      setBag(1, true, false);
      setBag(3, true, false);
    }
  }
}

void updateRunning() {
  unsigned long now = millis();
  if (runState == RUN_IDLE) {
    clearBags();

  } else if (runState == RUN_FILL || runState == RUN_DRAIN) {
    if (now - manualStartedAt >= MANUAL_MAX_MS) {
      stopManual();
      Serial.println(F("MANUAL LIMIT: 5 SECONDS"));
      return;
    }
    for (byte i = 0; i < 4; i++) {
      setBag(i, runState == RUN_FILL, runState == RUN_DRAIN);
    }

  } else if (runState == RUN_SHOW) {
    unsigned long elapsed = now - showStartedAt;
    if (elapsed >= SHOW_TOTAL_MS) {
      stopAll();
      Serial.println(F("SHOW FINISHED"));
      return;
    }
    updateShow(elapsed);
  }

  applyMaster();
  byte mode = MODE_STOP;
  if (runState == RUN_FILL) mode = MODE_FILL;
  else if (runState == RUN_DRAIN) mode = MODE_DRAIN;
  else if (runState == RUN_SHOW) mode = growthEnabled ? MODE_GROW : MODE_HOLD;

  // Heartbeat to Slave (watchdog = 400 ms).
  static unsigned long lastSentAt = 0;
  if (now - lastSentAt >= 50UL) {
    lastSentAt = now;
    sendSlave(mode);
  }
}

// ----------------------- GESTURES -----------------------
const byte TOUCH_NONE = 0;
const byte TOUCH_LEFT = 1;
const byte TOUCH_RIGHT = 2;
const byte TOUCH_BOTH = 3;
byte currentGesture = TOUCH_NONE;

bool bothSequence = false;
bool bothFirstRelease = false;
bool bothStopped = false;
unsigned long bothStartedAt = 0;
unsigned long bothReleasedAt = 0;
bool suppressGestureUntilRelease = false;

const char* touchName(byte gesture) {
  if (gesture == TOUCH_LEFT) return "LEFT";
  if (gesture == TOUCH_RIGHT) return "RIGHT";
  if (gesture == TOUCH_BOTH) return "BOTH";
  return "NONE";
}

void updateGestures() {
  unsigned long now = millis();
  debounceTouch(0, digitalRead(LEFT_PIN) == HIGH, now);
  debounceTouch(1, digitalRead(RIGHT_PIN) == HIGH, now);

  byte next = TOUCH_NONE;
  if (touchStable[0] && touchStable[1]) next = TOUCH_BOTH;
  else if (touchStable[0]) next = TOUCH_LEFT;
  else if (touchStable[1]) next = TOUCH_RIGHT;

  // After the serial STOP command, require all fingers to release.
  if (suppressGestureUntilRelease) {
    currentGesture = next;
    if (next == TOUCH_NONE) {
      suppressGestureUntilRelease = false;
      bothSequence = false;
      bothFirstRelease = false;
      bothStopped = false;
    }
    return;
  }

  // Start a BOTH gesture only when both fingers have debounced.
  if (next == TOUCH_BOTH && !bothSequence) {
    stopManual();
    bothSequence = true;
    bothFirstRelease = false;
    bothStopped = false;
    bothStartedAt = now;
  }

  // Software stop after BOTH is held for 6 seconds.
  if (next == TOUCH_BOTH && bothSequence && !bothStopped &&
      now - bothStartedAt >= BOTH_STOP_MS) {
    stopAll();
    bothStopped = true;
    Serial.println(F("BOTH 6S STOP"));
  }

  // Retain BOTH duration when one finger releases first.
  if (currentGesture == TOUCH_BOTH && next != TOUCH_BOTH &&
      bothSequence && !bothFirstRelease) {
    bothReleasedAt = now;
    bothFirstRelease = true;
  }

  if (next == currentGesture) return;

  Serial.print(F("TOUCH: "));
  Serial.println(touchName(next));

  if (next == TOUCH_NONE) {
    if (bothSequence) {
      unsigned long endedAt = bothFirstRelease ? bothReleasedAt : now;
      unsigned long holdMs = endedAt - bothStartedAt;
      if (!bothStopped && holdMs >= BOTH_SHOW_MIN_MS &&
          holdMs <= BOTH_SHOW_MAX_MS) {
        startShow();
      }
      bothSequence = false;
      bothFirstRelease = false;
      bothStopped = false;
    } else {
      stopManual();
    }

  } else if (currentGesture == TOUCH_NONE && !bothSequence) {
    // New single-hand hold: activate immediately after debounce.
    if (next == TOUCH_LEFT) startDrain();
    else if (next == TOUCH_RIGHT) startFill();

  } else if (next == TOUCH_BOTH) {
    // Transition from single to BOTH: stop single-hand pumping.
    stopManual();

  } else if (bothSequence) {
    // One finger released from BOTH: wait for the other to release.
    stopManual();

  } else {
    // Direct LEFT->RIGHT or RIGHT->LEFT without a full release.
    stopManual();
  }

  currentGesture = next;
}

// ----------------------- SERIAL -----------------------
// C/D: 5-second timed diagnostic run, independent of finger contact.
// S: stop; R: 180-second show; G: disable Stage II growth bias.
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= 'a' && c <= 'z') c -= 32;

    if (c == 'S') {
      stopAll();
      suppressGestureUntilRelease = true;
    } else if (c == 'C') {
      startFill();
    } else if (c == 'D') {
      startDrain();
    } else if (c == 'R') {
      startShow();
    } else if (c == 'G') {
      disableGrowth();
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LEFT_PIN, INPUT);
  pinMode(RIGHT_PIN, INPUT);

  const byte pins[12] = {
    B1_FILL_EN, B1_FILL_IN1, B1_FILL_IN2,
    B1_DRAIN_EN, B1_DRAIN_IN1, B1_DRAIN_IN2,
    B2_FILL_EN, B2_FILL_IN1, B2_FILL_IN2,
    B2_DRAIN_EN, B2_DRAIN_IN1, B2_DRAIN_IN2
  };
  for (byte i = 0; i < 12; i++) {
    digitalWrite(pins[i], LOW);
    pinMode(pins[i], OUTPUT);
  }
  clearBags();
  applyMaster();

  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000UL, true);
#endif

  delay(500);
  Wire.beginTransmission(SLAVE_ADDRESS);
  byte error = Wire.endTransmission();
  if (error == 0) Serial.println(F("SLAVE FOUND: 0x08"));
  else Serial.println(F("SLAVE NOT FOUND (BAGS 3/4 UNAVAILABLE)"));

  Serial.println(F("UTERUS MASTER V17.4 READY"));
  Serial.println(F("LEFT HOLD=DRAIN; RIGHT HOLD=FILL; MAX 5S"));
  Serial.println(F("BOTH 2-5S RELEASE=SHOW; BOTH 6S=STOP"));
  Serial.println(F("SERIAL C/D=TIMED 5S; R=SHOW; G=NO GROWTH; S=STOP"));
  // Let TTP223 finish power-on calibration without touching electrodes.
  delay(1000);
}

void loop() {
  handleSerial();
  updateGestures();
  updateRunning();
  delay(10);
}
