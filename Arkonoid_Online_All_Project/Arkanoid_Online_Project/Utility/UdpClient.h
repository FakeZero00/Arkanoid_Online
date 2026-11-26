#pragma once

#include "../../Network/UdpPositionProtocol.h"
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

class UdpClient {
public:
    static constexpr auto SendInterval = std::chrono::milliseconds(33);

    ~UdpClient();
    bool Start(const char* serverIp);
    void QueuePosition(std::uint32_t objectId, float x, float y);
    bool TryReceive(udp::Position& position);

private:
    void sendLocalPositions(std::stop_token stop);
    void receiveRemotePositions(std::stop_token stop);

    bool winsockStarted_ = false;
    SOCKET socket_ = INVALID_SOCKET;
    std::mutex sendMutex_;
    std::mutex receiveMutex_;
    std::queue<udp::PositionPacket> pendingPositions_;
    std::queue<udp::Position> receivedPositions_;
    std::jthread sendThread_;
    std::jthread receiveThread_;
};
