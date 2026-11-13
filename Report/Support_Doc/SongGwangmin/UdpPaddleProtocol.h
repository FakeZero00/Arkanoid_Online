#pragma once

#include <winsock2.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

constexpr unsigned short ServerPort = 9000;
constexpr auto UdpOnlyPos = std::uint32_t{50};

struct PaddlePosition {
    std::uint32_t sequence;
    float x;
    float y;
};

struct PositionPacket {
    std::uint32_t type;
    std::uint32_t sequence;
    std::uint32_t xBits;
    std::uint32_t yBits;
};

static_assert(sizeof(PositionPacket) == 16, "Unexpected UDP position packet size");
static_assert(sizeof(float) == sizeof(std::uint32_t) &&
    std::numeric_limits<float>::is_iec559, "UDP positions require IEEE 754 binary32");

inline PositionPacket encodePosition(const PaddlePosition& position) {
    std::uint32_t xBits, yBits;
    std::memcpy(&xBits, &position.x, sizeof(xBits));
    std::memcpy(&yBits, &position.y, sizeof(yBits));
    return {htonl(UdpOnlyPos), htonl(position.sequence), htonl(xBits), htonl(yBits)};
}

inline bool decodePosition(const PositionPacket& packet, PaddlePosition& position) {
    if (ntohl(packet.type) != UdpOnlyPos) return false;

    const std::uint32_t xBits = ntohl(packet.xBits);
    const std::uint32_t yBits = ntohl(packet.yBits);
    position.sequence = ntohl(packet.sequence);
    std::memcpy(&position.x, &xBits, sizeof(position.x));
    std::memcpy(&position.y, &yBits, sizeof(position.y));
    return std::isfinite(position.x) && std::isfinite(position.y);
}

inline bool isNewerSequence(std::uint32_t sequence, std::uint32_t previous) {
    const auto distance = sequence - previous;
    return distance != 0 && distance < 0x80000000u;
}
