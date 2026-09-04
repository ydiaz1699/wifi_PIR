#pragma once

#include "Arduino.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

class File {
public:
    File();
    File(const std::string& path, const char* mode);
    ~File();

    explicit operator bool() const { return valid_; }
    bool available() const;
    String readStringUntil(char delimiter);
    int read(uint8_t* destination, std::size_t length);
    std::size_t write(const uint8_t* data, std::size_t length);
    std::size_t print(const String& value);
    int printf(const char* format, ...);
    void println(uint32_t value);
    void close();

private:
    std::string path_;
    std::string data_;
    std::size_t position_;
    bool valid_;
    bool writable_;
};

struct FSInfo {
    std::size_t totalBytes;
    std::size_t usedBytes;
};

class LittleFSClass {
public:
    bool begin();
    bool format();
    bool exists(const char* path) const;
    bool mkdir(const char* path);
    File open(const char* path, const char* mode);
    bool remove(const char* path);
    bool rename(const char* from, const char* to);
    void info(FSInfo& info) const;

    bool failWrites = false;
    int failBegins = 0;

public:
    std::map<std::string, std::string> files_;
    bool directoryExists_ = false;
};

extern LittleFSClass LittleFS;

namespace HostLittleFS {
void reset();
void truncate(const char* path, std::size_t length);
std::size_t size(const char* path);
void setFailWrites(bool fail);
void setFailBegins(int count);
}
