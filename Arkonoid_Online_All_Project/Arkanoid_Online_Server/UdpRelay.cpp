#include "UdpRelay.h"
#include "UdpPositionProtocol.h"
#include <windows.h>
#include <ws2tcpip.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <unordered_map>

#pragma comment(lib, "ws2_32")

namespace {
using Clock = std::chrono::steady_clock;
constexpr auto PlayerTimeout = std::chrono::seconds(3);

SOCKET udpSocket = INVALID_SOCKET;
std::atomic<bool> running{false};
HANDLE relayThread = nullptr;

struct Player {
    sockaddr_in address{};
    std::unordered_map<std::uint32_t, udp::Position> positions;
    Clock::time_point lastReceived{};
    bool registered = false;
};

const Player* finddest_udp(const std::array<Player, 2>& players, const sockaddr_in& sender,
    int& senderIndex) {
    for (int playerindex = 0; playerindex < static_cast<int>(players.size()); ++playerindex) {
        const Player& player = players[playerindex];
        if (player.registered && player.address.sin_addr.s_addr == sender.sin_addr.s_addr &&
            player.address.sin_port == sender.sin_port) {
            senderIndex = playerindex;
            return &players[!playerindex];
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

DWORD WINAPI relayPositions(LPVOID) {
    std::array<Player, 2> players{};
    std::uint32_t relaySequence = 0;

    std::printf("[UDP] Listening for two players on %u\n", udp::RelayPort);
    std::fflush(stdout);

    while (running) {
        udp::PositionPacket packet{};
        sockaddr_in sender{};
        int senderLength = sizeof(sender);
        const int receivedBytes = recvfrom(udpSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0,
            reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (receivedBytes == SOCKET_ERROR) continue;

        udp::Position position{};
        if (receivedBytes != static_cast<int>(sizeof(packet)) || !udp::decodePosition(packet, position)) continue;

        const auto now = Clock::now();
        for (auto& player : players) {
            if (player.registered && now - player.lastReceived >= PlayerTimeout) {
                player = {};
            }
        }

        int senderIndex = -1;
        const Player* destination = finddest_udp(players, sender, senderIndex);
        const bool joining = destination == nullptr;
        if (joining && registerPlayer(players, sender)) {
            destination = finddest_udp(players, sender, senderIndex);
        }
        if (destination == nullptr) continue;

        Player& player = players[senderIndex];
        const auto [entry, firstPosition] = player.positions.try_emplace(position.objectId, position);
        auto& previous = entry->second;
        if (!firstPosition && !udp::isNewerSequence(position.sequence, previous.sequence)) continue;

        const bool changed = firstPosition || position.x != previous.x || position.y != previous.y;
        previous = position;
        player.lastReceived = now;

        const char name = senderIndex == 0 ? 'A' : 'B';
        const char destinationName = destination == &players[0] ? 'A' : 'B';
        if (joining) {
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));
            std::printf("[UDP join] %c %s:%u\n", name, ip, ntohs(sender.sin_port));
            std::fflush(stdout);
        }

        if (!destination->registered) continue;

        // Use a server sequence so a replacement player's first packet is newer too.
        const auto outgoing = udp::encodePosition({++relaySequence, position.objectId, position.x, position.y});
        const int sentBytes = sendto(udpSocket, reinterpret_cast<const char*>(&outgoing), sizeof(outgoing), 0,
            reinterpret_cast<const sockaddr*>(&destination->address), sizeof(destination->address));
        if (sentBytes == SOCKET_ERROR) {
            std::fprintf(stderr, "UDP sendto() to %c failed (%d)\n", destinationName, WSAGetLastError());
        } else if (changed) {
            std::printf("[UDP relay] %c -> %c id=%u seq=%u x=%.6f y=%.6f\n",
                name, destinationName, position.objectId, relaySequence, position.x, position.y);
            std::fflush(stdout);
        }
    }
    return 0;
}
}

void StopUdpRelay() {
    running = false;
    if (relayThread) {
        WaitForSingleObject(relayThread, INFINITE);
        CloseHandle(relayThread);
        relayThread = nullptr;
    }
    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }
}

bool StartUdpRelay() {
    if (udpSocket != INVALID_SOCKET) return false;
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::fprintf(stderr, "UDP socket() failed (%d)\n", WSAGetLastError());
        return false;
    }

    const DWORD receiveTimeout = 100;
    const BOOL exclusive = TRUE;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&receiveTimeout), sizeof(receiveTimeout)) == SOCKET_ERROR ||
        setsockopt(udpSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP setsockopt() failed (%d)\n", WSAGetLastError());
        StopUdpRelay();
        return false;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(udp::RelayPort);
    const int bindResult = bind(udpSocket, reinterpret_cast<const sockaddr*>(&server), sizeof(server));
    if (bindResult == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP bind() on %u failed (%d)\n", udp::RelayPort, WSAGetLastError());
        StopUdpRelay();
        return false;
    }

    running = true;
    relayThread = CreateThread(nullptr, 0, relayPositions, nullptr, 0, nullptr);
    if (!relayThread) {
        std::fprintf(stderr, "UDP relay CreateThread() failed (%lu)\n", GetLastError());
        StopUdpRelay();
        return false;
    }
    return true;
}
