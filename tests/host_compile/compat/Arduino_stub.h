#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <vector>

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

class SerialClass {
public:
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        std::vfprintf(stderr, format, args);
        va_end(args);
    }
};

extern SerialClass Serial;

namespace HostUdp {

void reset();
void inject(const uint8_t* data, std::size_t length, uint16_t port = 4210);
std::size_t sentCount();
bool copySent(std::size_t index, uint8_t* destination,
              std::size_t capacity, std::size_t& length);

}  // namespace HostUdp

class WiFiUDP {
public:
    void begin(uint16_t) {}
    int parsePacket();
    int read(uint8_t*, std::size_t);
    IPAddress remoteIP() { return IPAddress(); }
    uint16_t remotePort() const;
    void beginPacket(IPAddress, uint16_t);
    std::size_t write(const uint8_t*, std::size_t length);
    void endPacket();
    void flush();
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
