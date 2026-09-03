#include "LittleFS.h"

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

LittleFSClass LittleFS;

void String::trim() {
    const std::size_t first = value_.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        value_.clear();
        return;
    }
    const std::size_t last = value_.find_last_not_of(" \t\r\n");
    value_ = value_.substr(first, last - first + 1);
}

int String::indexOf(char needle) const {
    const std::size_t position = value_.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
}

String String::substring(int start) const {
    if (start < 0 || static_cast<std::size_t>(start) >= value_.size()) return String();
    return String(value_.substr(static_cast<std::size_t>(start)));
}

String String::substring(int start, int end) const {
    if (start < 0 || end < start || static_cast<std::size_t>(start) >= value_.size()) {
        return String();
    }
    const std::size_t count = std::min<std::size_t>(
        static_cast<std::size_t>(end - start), value_.size() - static_cast<std::size_t>(start));
    return String(value_.substr(static_cast<std::size_t>(start), count));
}

long String::toInt() const {
    char* end = nullptr;
    const long result = std::strtol(value_.c_str(), &end, 10);
    return end == value_.c_str() ? 0 : result;
}

void String::toCharArray(char* destination, unsigned int capacity) const {
    if (capacity == 0) return;
    const std::size_t count = std::min<std::size_t>(value_.size(), capacity - 1);
    std::memcpy(destination, value_.data(), count);
    destination[count] = '\0';
}

String IPAddress::toString() const {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u",
                  bytes_[0], bytes_[1], bytes_[2], bytes_[3]);
    return String(buffer);
}

bool IPAddress::fromString(const String& value) {
    unsigned int a = 0, b = 0, c = 0, d = 0;
    char tail = 0;
    if (std::sscanf(value.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
    }
    bytes_[0] = static_cast<uint8_t>(a);
    bytes_[1] = static_cast<uint8_t>(b);
    bytes_[2] = static_cast<uint8_t>(c);
    bytes_[3] = static_cast<uint8_t>(d);
    return true;
}

File::File()
    : position_(0), valid_(false), writable_(false) {}

File::File(const std::string& path, const char* mode)
    : path_(path), position_(0), valid_(false), writable_(mode && mode[0] == 'w') {
    if (writable_) {
        if (LittleFS.failWrites) return;
        valid_ = true;
    } else {
        const auto it = LittleFS.files_.find(path_);
        if (it == LittleFS.files_.end()) return;
        data_ = it->second;
        valid_ = true;
    }
}

File::~File() {
    close();
}

bool File::available() const {
    return valid_ && position_ < data_.size();
}

String File::readStringUntil(char delimiter) {
    if (!valid_) return String();
    const std::size_t end = data_.find(delimiter, position_);
    const std::size_t count = end == std::string::npos ? data_.size() - position_ : end - position_;
    const String result(data_.substr(position_, count));
    position_ += count;
    if (end != std::string::npos) ++position_;
    return result;
}

int File::read(uint8_t* destination, std::size_t length) {
    if (!valid_) return 0;
    const std::size_t count = std::min(length, data_.size() - position_);
    std::memcpy(destination, data_.data() + position_, count);
    position_ += count;
    return static_cast<int>(count);
}

std::size_t File::write(const uint8_t* data, std::size_t length) {
    if (!valid_ || !writable_) return 0;
    data_.append(reinterpret_cast<const char*>(data), length);
    return length;
}

std::size_t File::print(const String& value) {
    return write(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
}

int File::printf(const char* format, ...) {
    char buffer[128]{};
    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (length <= 0) return length;
    const std::size_t written = write(reinterpret_cast<const uint8_t*>(buffer),
                                      std::min<std::size_t>(length, sizeof(buffer) - 1));
    return static_cast<int>(written);
}

void File::println(uint32_t value) {
    const String line(value);
    print(line);
    write(reinterpret_cast<const uint8_t*>("\n"), 1);
}

void File::close() {
    if (!valid_) return;
    if (writable_) LittleFS.files_[path_] = data_;
    valid_ = false;
}

bool LittleFSClass::begin() { return true; }

bool LittleFSClass::format() {
    files_.clear();
    directoryExists_ = false;
    return true;
}

bool LittleFSClass::exists(const char* path) const {
    if (std::string(path) == "/iot") return directoryExists_;
    return files_.find(path) != files_.end();
}

bool LittleFSClass::mkdir(const char* path) {
    if (std::string(path) != "/iot") return false;
    directoryExists_ = true;
    return true;
}

File LittleFSClass::open(const char* path, const char* mode) {
    return File(path, mode);
}

bool LittleFSClass::remove(const char* path) {
    return files_.erase(path) > 0;
}

bool LittleFSClass::rename(const char* from, const char* to) {
    const auto it = files_.find(from);
    if (it == files_.end()) return false;
    files_[to] = it->second;
    files_.erase(it);
    return true;
}

void LittleFSClass::info(FSInfo& info) const {
    info.totalBytes = 1024 * 1024;
    info.usedBytes = 0;
    for (const auto& entry : files_) info.usedBytes += entry.second.size();
}

namespace HostLittleFS {

void reset() {
    LittleFS.format();
    LittleFS.failWrites = false;
}

void truncate(const char* path, std::size_t length) {
    auto it = LittleFS.files_.find(path);
    if (it != LittleFS.files_.end()) it->second.resize(std::min(length, it->second.size()));
}

std::size_t size(const char* path) {
    const auto it = LittleFS.files_.find(path);
    return it == LittleFS.files_.end() ? 0 : it->second.size();
}

void setFailWrites(bool fail) {
    LittleFS.failWrites = fail;
}

}  // namespace HostLittleFS
