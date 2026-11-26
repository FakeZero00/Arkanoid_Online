#include "UdpClient.h"
#include <ws2tcpip.h>
#include <cstdio>
#include <system_error>

#pragma comment(lib, "ws2_32")

namespace {
constexpr auto PollInterval = std::chrono::milliseconds(4);
constexpr std::size_t MaxPendingPositions = 256;
}

UdpClient::~UdpClient() {
    sendThread_.request_stop();
    receiveThread_.request_stop();
    if (sendThread_.joinable()) sendThread_.join();
    if (receiveThread_.joinable()) receiveThread_.join();
    if (socket_ != INVALID_SOCKET) closesocket(socket_);
    if (winsockStarted_) WSACleanup();
}

bool UdpClient::Start(const char* serverIp) {
    if (winsockStarted_) return false;
    WSADATA wsa{};
    const int startup = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startup != 0) {
        std::fprintf(stderr, "UDP WSAStartup() failed (%d)\n", startup);
        return false;
    }
    winsockStarted_ = true;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(udp::RelayPort);
    if (inet_pton(AF_INET, serverIp, &server.sin_addr) != 1) {
        std::fprintf(stderr, "Invalid UDP server IPv4 address: %s\n", serverIp);
        return false;
    }

    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        std::fprintf(stderr, "UDP socket() failed (%d)\n", WSAGetLastError());
        return false;
    }
    const int connected = connect(socket_, reinterpret_cast<const sockaddr*>(&server), sizeof(server));
    u_long nonBlocking = 1;
    if (connected == SOCKET_ERROR || ioctlsocket(socket_, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        std::fprintf(stderr, "UDP socket setup failed (%d)\n", WSAGetLastError());
        return false;
    }

    try {
        receiveThread_ = std::jthread([this](std::stop_token stop) { receiveRemotePositions(stop); });
        sendThread_ = std::jthread([this](std::stop_token stop) { sendLocalPositions(stop); });
    } catch (const std::system_error& error) {
        std::fprintf(stderr, "Could not start UDP client threads (%s)\n", error.what());
        return false;
    }
    return true;
}

void UdpClient::QueuePosition(std::uint32_t objectId, float x, float y) {
    std::lock_guard lock(sendMutex_);
    if (pendingPositions_.size() == MaxPendingPositions) pendingPositions_.pop();
    pendingPositions_.push(udp::encodePosition({ 0, objectId, x, y }));
}

bool UdpClient::TryReceive(udp::Position& position) {
    std::lock_guard lock(receiveMutex_);
    if (receivedPositions_.empty()) return false;
    position = receivedPositions_.front();
    receivedPositions_.pop();
    return true;
}

void UdpClient::sendLocalPositions(std::stop_token stop) {
    std::uint32_t nextSequence = 1;
    while (!stop.stop_requested()) {
        {
            std::lock_guard lock(sendMutex_);
            while (!pendingPositions_.empty() && !stop.stop_requested()) {
                auto& packet = pendingPositions_.front();
                packet.sequence = htonl(nextSequence);
                const int sentBytes = send(socket_, reinterpret_cast<const char*>(&packet), sizeof(packet), 0);
                if (sentBytes == SOCKET_ERROR) {
                    const int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) break;
                    std::fprintf(stderr, "UDP send() failed (%d)\n", error);
                    return;
                }
                pendingPositions_.pop();
                ++nextSequence;
            }
        }
        std::this_thread::sleep_for(PollInterval);
    }
}

void UdpClient::receiveRemotePositions(std::stop_token stop) {
    while (!stop.stop_requested()) {
        udp::PositionPacket packet{};
        const int receivedBytes = recv(socket_, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
        if (receivedBytes == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) {
                std::this_thread::sleep_for(PollInterval);
                continue;
            }
            if (error == WSAEMSGSIZE) continue;
            std::fprintf(stderr, "UDP recv() failed (%d)\n", error);
            return;
        }

        udp::Position position{};
        if (receivedBytes != static_cast<int>(sizeof(packet)) || !udp::decodePosition(packet, position)) continue;
        std::lock_guard lock(receiveMutex_);
        if (receivedPositions_.size() == MaxPendingPositions) receivedPositions_.pop();
        receivedPositions_.push(position);
    }
}
