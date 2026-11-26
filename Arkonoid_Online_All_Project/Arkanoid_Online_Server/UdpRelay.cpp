#include "UdpRelay.h"
#include "../Network/UdpPositionProtocol.h"
#include <ws2tcpip.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <system_error>
#include <unordered_map>

#pragma comment(lib, "ws2_32")

namespace {
using Clock = std::chrono::steady_clock;
constexpr auto PlayerTimeout = std::chrono::seconds(3);

struct Player {
    sockaddr_in address{};
    std::unordered_map<std::uint32_t, udp::Position> positions;
    Clock::time_point lastReceived{};
    bool registered = false;
};

const Player* finddest_udp(const std::array<Player, 2>& players, const sockaddr_in& sender,
    int& playerIndex) {
    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        const auto& player = players[i];
        if (player.registered && player.address.sin_addr.s_addr == sender.sin_addr.s_addr &&
            player.address.sin_port == sender.sin_port) {
            playerIndex = i;
            return &players[!playerIndex];
        }
    }
    return nullptr;
}

bool registerPlayer(std::array<Player, 2>& players, const sockaddr_in& sender) {
    for (auto& player : players) {
        if (!player.registered) {
            player.address = sender;
            player.registered = true;
            return true;
        }
    }
    return false;
}
}

UdpRelay::~UdpRelay() {
    Stop();
}

void UdpRelay::Stop() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool UdpRelay::Start() {
    if (socket_ != INVALID_SOCKET) return false;
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        std::fprintf(stderr, "UDP socket() failed (%d)\n", WSAGetLastError());
        return false;
    }

    const DWORD receiveTimeout = 100;
    const BOOL exclusive = TRUE;
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&receiveTimeout), sizeof(receiveTimeout)) == SOCKET_ERROR ||
        setsockopt(socket_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP setsockopt() failed (%d)\n", WSAGetLastError());
        return false;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(udp::RelayPort);
    const int bound = bind(socket_, reinterpret_cast<const sockaddr*>(&server), sizeof(server));
    if (bound == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP bind() on %u failed (%d)\n", udp::RelayPort, WSAGetLastError());
        return false;
    }

    try {
        thread_ = std::jthread([this](std::stop_token stop) { relayPositions(stop); });
    } catch (const std::system_error& error) {
        std::fprintf(stderr, "Could not start the UDP relay (%s)\n", error.what());
        return false;
    }
    return true;
}

void UdpRelay::relayPositions(std::stop_token stop) const {
    std::array<Player, 2> players{};
    std::uint32_t relaySequence = 0;
    std::printf("[UDP] Listening for two players on %u\n", udp::RelayPort);
    std::fflush(stdout);

    while (!stop.stop_requested()) {
        udp::PositionPacket packet{};
        sockaddr_in sender{};
        int senderLength = sizeof(sender);
        const int receivedBytes = recvfrom(socket_, reinterpret_cast<char*>(&packet), sizeof(packet), 0,
            reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (receivedBytes == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEMSGSIZE || error == WSAECONNRESET) continue;
            std::fprintf(stderr, "UDP recvfrom() failed (%d)\n", error);
            return;
        }

        udp::Position position{};
        if (receivedBytes != static_cast<int>(sizeof(packet)) || !udp::decodePosition(packet, position)) continue;
        const auto now = Clock::now();
        for (auto& player : players) {
            if (player.registered && now - player.lastReceived >= PlayerTimeout) player = {};
        }

        int playerIndex = -1;
        const Player* destination = finddest_udp(players, sender, playerIndex);
        const bool joining = destination == nullptr;
        if (joining && registerPlayer(players, sender)) {
            destination = finddest_udp(players, sender, playerIndex);
        }
        if (!destination) continue;

        auto& player = players[playerIndex];
        const auto previous = player.positions.find(position.objectId);
        if (previous != player.positions.end() &&
            !udp::isNewerSequence(position.sequence, previous->second.sequence)) continue;
        const bool changed = previous == player.positions.end() ||
            previous->second.x != position.x || previous->second.y != position.y;
        player.positions.insert_or_assign(position.objectId, position);
        player.lastReceived = now;

        const char sourceName = playerIndex == 0 ? 'A' : 'B';
        const char destinationName = playerIndex == 0 ? 'B' : 'A';
        if (joining) {
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));
            std::printf("[UDP join] %c %s:%u\n", sourceName, ip, ntohs(sender.sin_port));
            std::fflush(stdout);
        }
        if (!destination->registered) continue;

        // A replacement peer still gets a sequence newer than the previous peer's.
        const auto outgoing = udp::encodePosition({ ++relaySequence, position.objectId, position.x, position.y });
        const int sentBytes = sendto(socket_, reinterpret_cast<const char*>(&outgoing), sizeof(outgoing), 0,
            reinterpret_cast<const sockaddr*>(&destination->address), sizeof(destination->address));
        if (sentBytes == SOCKET_ERROR) {
            std::fprintf(stderr, "UDP sendto() to %c failed (%d)\n", destinationName, WSAGetLastError());
        } else if (changed) {
            std::printf("[UDP relay] %c -> %c id=%u seq=%u x=%.6f y=%.6f\n",
                sourceName, destinationName, position.objectId, relaySequence, position.x, position.y);
            std::fflush(stdout);
        }
    }
}
