#pragma once

#include "UdpPositionProtocol.h"
#include <chrono>
#include <mutex>

class Transform;

inline constexpr auto UdpSendInterval = std::chrono::milliseconds(33);

extern std::mutex positionMutex;

bool StartUdpClient(const char* serverIp, const udp::Position& initialBous, std::uint32_t localBallId,
    Transform* opponentBous, Transform* opponentBall);
void StopUdpClient();
void QueueUdpPosition(std::uint32_t objectId, float x, float y);
