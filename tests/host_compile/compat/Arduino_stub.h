#pragma once

#include <cstddef>
#include <cstdint>

using ulong_t = unsigned long;

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void randomSeed(long) {}
inline long random(long minimum, long) { return minimum; }
inline int analogRead(int) { return 0; }
inline void delay(unsigned long) {}

#define A0 0
#define WL_CONNECTED 1

class String {
public:
    String() = default;
    String(const char*) {}
};

class IPAddress {
public:
    IPAddress() = default;
    IPAddress(int, int, int, int) {}

    String toString() const { return String(); }
    bool fromString(const String&) { return true; }
};

class WiFiUDP {
public:
    void begin(uint16_t) {}
    int parsePacket() { return 0; }
    int read(uint8_t*, std::size_t) { return 0; }
    IPAddress remoteIP() { return IPAddress(); }
    uint16_t remotePort() { return 0; }
    void beginPacket(IPAddress, uint16_t) {}
    std::size_t write(const uint8_t*, std::size_t length) { return length; }
    void endPacket() {}
    void flush() {}
};

class WiFiClass {
public:
    int status() const { return WL_CONNECTED; }
    int RSSI() const { return -50; }
};

class ESPClass {
public:
    uint32_t getFreeHeap() const { return 40000; }
    void restart() {}
};

extern WiFiClass WiFi;
extern ESPClass ESP;
