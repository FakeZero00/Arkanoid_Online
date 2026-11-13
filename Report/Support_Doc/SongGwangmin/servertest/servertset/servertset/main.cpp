#include "Common.h"
#include "../../../UdpPaddleProtocol.h"

#include <array>
#include <chrono>

using Clock = std::chrono::steady_clock;
constexpr auto PlayerTimeout = std::chrono::seconds(3);

struct Player {
    sockaddr_in address{};
    PaddlePosition position{};
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

int relayPositions(SOCKET sock) {
    std::array<Player, 2> players{};
    std::uint32_t relaySequence = 0;

    printf("Listening for two players on UDP port %u\n", ServerPort);
    fflush(stdout);

    while (true) {
        PositionPacket packet{};
        sockaddr_in sender{};
        int senderLength = sizeof(sender);
        const int receivedBytes = recvfrom(sock, reinterpret_cast<char*>(&packet), sizeof(packet), 0,
            reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (receivedBytes == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAEMSGSIZE || error == WSAECONNRESET) continue;
            fprintf(stderr, "recvfrom() failed (%d)\n", error);
            return 1;
        }

        PaddlePosition position{};
        if (receivedBytes != static_cast<int>(sizeof(packet)) || !decodePosition(packet, position)) continue;

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
        if (!joining && !isNewerSequence(position.sequence, player.position.sequence)) continue;

        const bool changed = joining || position.x != player.position.x || position.y != player.position.y;
        player.position = position;
        player.lastReceived = now;

        const char name = senderIndex == 0 ? 'A' : 'B';
        const char destinationName = destination == &players[0] ? 'A' : 'B';

        if (joining) {
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));
            printf("[join] %c %s:%u\n", name, ip, ntohs(sender.sin_port));
            fflush(stdout);
        }

        if (!destination->registered) continue;

        // Use a server sequence so a replacement player's first packet is newer too.
        const PositionPacket outgoing = encodePosition({++relaySequence, position.x, position.y});
        const int sentBytes = sendto(sock, reinterpret_cast<const char*>(&outgoing), sizeof(outgoing), 0,
            reinterpret_cast<const sockaddr*>(&destination->address), sizeof(destination->address));
        if (sentBytes == SOCKET_ERROR) {
            fprintf(stderr, "sendto() to %c failed (%d)\n", destinationName, WSAGetLastError());
        } else if (changed) {
            printf("[relay] %c -> %c clientSeq=%u relaySeq=%u x=%.6f y=%.6f\n",
                name, destinationName, position.sequence, relaySequence, position.x, position.y);
            fflush(stdout);
        }
    }
}

int main() {
    WSADATA wsa;
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startupResult != 0) return 1;

    const SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed (%d)\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(ServerPort);

    int exitCode = 1;
    const int bindResult = bind(sock, reinterpret_cast<const sockaddr*>(&server), sizeof(server));
    if (bindResult == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed (%d)\n", WSAGetLastError());
    } else {
        exitCode = relayPositions(sock);
    }

    closesocket(sock);
    WSACleanup();
    return exitCode;
}
