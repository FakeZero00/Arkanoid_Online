#pragma once

#include <winsock2.h>
#include <thread>

class UdpRelay {
public:
    ~UdpRelay();
    bool Start(); // Winsock must be initialized by the server.
    void Stop();

private:
    void relayPositions(std::stop_token stop) const;

    SOCKET socket_ = INVALID_SOCKET;
    std::jthread thread_;
};
