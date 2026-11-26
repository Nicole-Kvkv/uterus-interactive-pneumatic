#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>

using byte = uint8_t;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define F(x) (x)
#define WIRE_HAS_TIMEOUT 1

extern unsigned long mockMillis;
extern int mockInput[32];
extern int mockOutput[32];
extern int mockPWM[32];
extern bool mockInterruptsEnabled;

inline unsigned long millis() { return mockMillis; }
inline void delay(unsigned long duration) { mockMillis += duration; }
inline int digitalRead(byte pin) { return mockInput[pin]; }
inline void digitalWrite(byte pin, byte value) { mockOutput[pin] = value; }
inline void analogWrite(byte pin, int value) { mockPWM[pin] = value; }
inline void pinMode(byte, byte) {}
inline void noInterrupts() { mockInterruptsEnabled = false; }
inline void interrupts() { mockInterruptsEnabled = true; }

class MockSerial {
public:
  std::string log;
  std::string pendingInput;
  void begin(long) {}
  int available() { return static_cast<int>(pendingInput.size()); }
  char read() {
    if (pendingInput.empty()) return 0;
    char c = pendingInput.front();
    pendingInput.erase(pendingInput.begin());
    return c;
  }
  template <typename T> void print(const T &value) {
    std::ostringstream out;
    out << value;
    log += out.str();
  }
  template <typename T> void println(const T &value) {
    print(value);
    log += '\n';
  }
  void println() { log += '\n'; }
};
extern MockSerial Serial;
