#include "Arduino_stub.h"

#include <algorithm>
#include <cstring>
#include <deque>

namespace {

struct Datagram {
    std::vector<uint8_t> data;
    uint16_t port;
};

std::deque<Datagram> incoming;
std::vector<uint8_t> current;
uint16_t currentPort = 0;
bool currentReady = false;
std::vector<std::vector<uint8_t>> sent;
std::vector<uint8_t> outgoing;

}  // namespace

WiFiClass WiFi;
ESPClass ESP;
SerialClass Serial;

namespace HostUdp {

void reset() {
    incoming.clear();
    current.clear();
    currentPort = 0;
    currentReady = false;
    sent.clear();
    outgoing.clear();
}

void inject(const uint8_t* data, std::size_t length, uint16_t port) {
    incoming.push_back({std::vector<uint8_t>(data, data + length), port});
}

std::size_t sentCount() {
    return sent.size();
}

bool copySent(std::size_t index, uint8_t* destination,
              std::size_t capacity, std::size_t& length) {
    if (index >= sent.size() || sent[index].size() > capacity) return false;
    length = sent[index].size();
    std::memcpy(destination, sent[index].data(), length);
    return true;
}

}  // namespace HostUdp

int WiFiUDP::parsePacket() {
    if (!currentReady && !incoming.empty()) {
        current = std::move(incoming.front().data);
        currentPort = incoming.front().port;
        incoming.pop_front();
        currentReady = true;
    }
    return currentReady ? static_cast<int>(current.size()) : 0;
}

int WiFiUDP::read(uint8_t* destination, std::size_t capacity) {
    if (!currentReady) return 0;
    const std::size_t length = std::min(capacity, current.size());
    std::memcpy(destination, current.data(), length);
    current.clear();
    currentReady = false;
    return static_cast<int>(length);
}

uint16_t WiFiUDP::remotePort() const {
    return currentPort;
}

void WiFiUDP::beginPacket(IPAddress, uint16_t) {
    outgoing.clear();
}

std::size_t WiFiUDP::write(const uint8_t* data, std::size_t length) {
    outgoing.assign(data, data + length);
    return length;
}

void WiFiUDP::endPacket() {
    sent.push_back(outgoing);
}

void WiFiUDP::flush() {
    current.clear();
    currentReady = false;
}
