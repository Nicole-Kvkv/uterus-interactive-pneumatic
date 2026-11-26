#pragma once
#include "Arduino.h"
#include <vector>

class MockWire {
public:
  byte slaveAddress = 0;
  byte targetAddress = 0;
  byte txAck = 0;
  std::vector<byte> txBuffer;
  std::vector<byte> lastPacket;
  std::vector<byte> rxBuffer;
  size_t rxIndex = 0;
  void (*receiveCallback)(int) = nullptr;
  void begin() {}
  void begin(byte slave) { slaveAddress = slave; }
  void onReceive(void (*callback)(int)) { receiveCallback = callback; }
  void setWireTimeout(unsigned long, bool) {}
  void beginTransmission(byte target) {
    targetAddress = target;
    txBuffer.clear();
  }
  size_t write(const byte *values, size_t count) {
    txBuffer.insert(txBuffer.end(), values, values + count);
    return count;
  }
  size_t write(byte value) { txBuffer.push_back(value); return 1; }
  byte endTransmission() { lastPacket = txBuffer; return txAck; }
  int available() { return static_cast<int>(rxBuffer.size() - rxIndex); }
  byte read() { return rxBuffer[rxIndex++]; }
  void injectReceive(const std::vector<byte> &packet) {
    rxBuffer = packet;
    rxIndex = 0;
    if (receiveCallback) receiveCallback(static_cast<int>(packet.size()));
  }
};
extern MockWire Wire;
