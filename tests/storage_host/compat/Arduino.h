#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#define A0 0

inline unsigned long micros() { return 1234; }
inline int analogRead(int) { return 7; }

class String {
public:
    String() = default;
    String(const char* value) : value_(value ? value : "") {}
    String(const std::string& value) : value_(value) {}
    String(unsigned long value) : value_(std::to_string(value)) {}
    String(uint32_t value) : value_(std::to_string(value)) {}
    String(uint16_t value) : value_(std::to_string(value)) {}
    String(uint8_t value) : value_(std::to_string(static_cast<unsigned>(value))) {}
    String(int value) : value_(std::to_string(value)) {}

    unsigned int length() const { return static_cast<unsigned int>(value_.length()); }
    char charAt(unsigned int index) const { return value_.at(index); }
    const char* c_str() const { return value_.c_str(); }
    void trim();
    int indexOf(char needle) const;
    String substring(int start) const;
    String substring(int start, int end) const;
    long toInt() const;
    void toCharArray(char* destination, unsigned int capacity) const;

    String& operator+=(const String& other) { value_ += other.value_; return *this; }
    String& operator+=(const char* other) { value_ += other ? other : ""; return *this; }
    String& operator+=(char other) { value_ += other; return *this; }
    bool operator==(const char* other) const { return value_ == (other ? other : ""); }

private:
    std::string value_;
};

class IPAddress {
public:
    IPAddress() : bytes_{0, 0, 0, 0} {}
    IPAddress(int a, int b, int c, int d)
        : bytes_{static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                 static_cast<uint8_t>(c), static_cast<uint8_t>(d)} {}

    String toString() const;
    bool fromString(const String& value);

private:
    uint8_t bytes_[4];
};
