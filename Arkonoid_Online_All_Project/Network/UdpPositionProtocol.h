#pragma once

#include <winsock2.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace udp {

constexpr unsigned short RelayPort = 9800;
constexpr auto UdpOnlyPos = std::uint32_t{50};

struct Position {
    std::uint32_t sequence;
    std::uint32_t objectId;
    float x;
    float y;
};

struct PositionPacket {
    std::uint32_t type;
    std::uint32_t sequence;
    std::uint32_t objectId;
    std::uint32_t xBits;
    std::uint32_t yBits;
};

static_assert(sizeof(PositionPacket) == 20, "Unexpected UDP position packet size");
static_assert(sizeof(float) == sizeof(std::uint32_t) &&
    std::numeric_limits<float>::is_iec559, "UDP positions require IEEE 754 binary32");

inline PositionPacket encodePosition(const Position& position) {
    std::uint32_t xBits, yBits;
    std::memcpy(&xBits, &position.x, sizeof(xBits));
    std::memcpy(&yBits, &position.y, sizeof(yBits));
    return {htonl(UdpOnlyPos), htonl(position.sequence), htonl(position.objectId), htonl(xBits), htonl(yBits)};
}

inline bool decodePosition(const PositionPacket& packet, Position& position) {
    if (ntohl(packet.type) != UdpOnlyPos) return false;
    const std::uint32_t xBits = ntohl(packet.xBits);
    const std::uint32_t yBits = ntohl(packet.yBits);
    position.sequence = ntohl(packet.sequence);
    position.objectId = ntohl(packet.objectId);
    std::memcpy(&position.x, &xBits, sizeof(xBits));
    std::memcpy(&position.y, &yBits, sizeof(yBits));
    return std::isfinite(position.x) && std::isfinite(position.y);
}

inline bool isNewerSequence(std::uint32_t sequence, std::uint32_t previous) {
    const auto distance = sequence - previous;
    return distance != 0 && distance < 0x80000000u;
}

} // namespace udp
