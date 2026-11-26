#include "UdpClient.h"
#include "Transform.h"
#include <windows.h>
#include <ws2tcpip.h>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <queue>

#pragma comment(lib, "ws2_32")

std::mutex positionMutex;

namespace {
using Clock = std::chrono::steady_clock;
constexpr auto PositionRefreshInterval = std::chrono::milliseconds(250);

bool winsockStarted = false;
SOCKET udpSocket = INVALID_SOCKET;
std::atomic<bool> running{false};
HANDLE sendThread = nullptr;
HANDLE receiveThread = nullptr;
std::mutex queueMutex;
std::queue<udp::PositionPacket> pendingPositions;
std::uint32_t bousId = 0;
std::uint32_t ballId = 0;
Transform* remoteBous = nullptr;
Transform* remoteBall = nullptr;
udp::PositionPacket initialBousPacket{};

DWORD WINAPI sendLocalPositions(LPVOID) {
    udp::PositionPacket lastSent = initialBousPacket;
    std::uint32_t nextSequence = 1;
    auto lastSend = Clock::now() - PositionRefreshInterval;

    while (running) {
        udp::PositionPacket packet = lastSent;
        bool fromQueue;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            fromQueue = !pendingPositions.empty();
            if (fromQueue) packet = pendingPositions.front();
        }
        if (!fromQueue && Clock::now() - lastSend < PositionRefreshInterval) continue;

        packet.sequence = htonl(nextSequence);

        const int sentBytes = send(udpSocket, reinterpret_cast<const char*>(&packet), sizeof(packet), 0);
        if (sentBytes == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) continue;
        } else {
            lastSent = packet;
            ++nextSequence;
            lastSend = Clock::now();
        }

        if (!fromQueue) continue;
        // Only this thread removes packets, so the front is still the one sent.
        std::lock_guard<std::mutex> lock(queueMutex);
        pendingPositions.pop();
    }

    return 0;
}

DWORD WINAPI receiveRemotePositions(LPVOID) {
    std::uint32_t lastBousSequence = 0;
    std::uint32_t lastBallSequence = 0;

    while (running) {
        udp::PositionPacket packet{};
        const int bytes = recv(udpSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
        if (bytes == SOCKET_ERROR) continue;

        udp::Position position{};
        if (bytes != static_cast<int>(sizeof(packet)) || !decodePosition(packet, position)) continue;

        std::lock_guard<std::mutex> lock(positionMutex);
        Transform* remote;
        if (position.objectId == bousId) {
            remote = remoteBous;
        } else if (position.objectId == ballId) {
            remote = remoteBall;
        } else {
            continue;
        }

        auto& lastSequence = remote == remoteBous ? lastBousSequence : lastBallSequence;
        if (position.sequence < lastSequence) continue;
        remote->SetLocalPosition(position.x, position.y, remote->position.z);
        lastSequence = position.sequence;
    }
    return 0;
}
}

void StopUdpClient() {
    running = false;
    for (HANDLE thread : { sendThread, receiveThread }) {
        if (!thread) continue;
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    sendThread = nullptr;
    receiveThread = nullptr;

    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }
    if (winsockStarted) {
        WSACleanup();
        winsockStarted = false;
    }

    while (!pendingPositions.empty()) pendingPositions.pop();
}

bool StartUdpClient(const char* serverIp, const udp::Position& initialBous, std::uint32_t localBallId,
    Transform* opponentBous, Transform* opponentBall) {
    if (winsockStarted) return false;
    WSADATA wsa{};
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startupResult != 0) {
        std::fprintf(stderr, "UDP WSAStartup() failed (%d)\n", startupResult);
        return false;
    }
    winsockStarted = true;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(udp::RelayPort);
    if (inet_pton(AF_INET, serverIp, &server.sin_addr) != 1) {
        std::fprintf(stderr, "Invalid UDP server IPv4 address: %s\n", serverIp);
        StopUdpClient();
        return false;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::fprintf(stderr, "UDP socket() failed (%d)\n", WSAGetLastError());
        StopUdpClient();
        return false;
    }

    u_long nonblocking = 1;
    const int connected = connect(udpSocket, reinterpret_cast<const sockaddr*>(&server), sizeof(server));
    if (connected == SOCKET_ERROR || ioctlsocket(udpSocket, FIONBIO, &nonblocking) == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP socket setup failed (%d)\n", WSAGetLastError());
        StopUdpClient();
        return false;
    }

    bousId = initialBous.objectId;
    ballId = localBallId;
    remoteBous = opponentBous;
    remoteBall = opponentBall;
    initialBousPacket = udp::encodePosition(initialBous);

    running = true;
    receiveThread = CreateThread(nullptr, 0, receiveRemotePositions, nullptr, 0, nullptr);
    if (receiveThread) sendThread = CreateThread(nullptr, 0, sendLocalPositions, nullptr, 0, nullptr);
    if (!sendThread) {
        std::fprintf(stderr, "UDP client CreateThread() failed (%lu)\n", GetLastError());
        StopUdpClient();
        return false;
    }
    return true;
}

void QueueUdpPosition(std::uint32_t objectId, float x, float y) {
    if (!running) return;
    std::lock_guard<std::mutex> lock(queueMutex);
    pendingPositions.push(udp::encodePosition({0, objectId, x, y}));
}
