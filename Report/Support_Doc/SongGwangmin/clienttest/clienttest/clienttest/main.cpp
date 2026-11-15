#include "Common.h"
#include "Vec2D.h"
#include "../../../UdpPaddleProtocol.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <system_error>
#include <thread>

using Clock = std::chrono::steady_clock;
constexpr auto SendInterval = std::chrono::milliseconds(33);
constexpr auto PositionRefreshInterval = std::chrono::milliseconds(250);
constexpr auto PeerTimeout = std::chrono::seconds(3);

namespace {
Vec2D localBous;
Vec2D remoteBous;
std::queue<PositionPacket> pendingPositions;
std::mutex queueMutex;
std::mutex positionMutex;
}

struct ClientState {
    std::atomic<bool> running{true};
    std::condition_variable displayReady;

    // Protected by positionMutex together with the two positions.
    bool displayDirty = true;
    bool hasRemotePosition = false;
};

void updatePosition(Vec2D& position, const Vec2D& direction, float deltaTime) {
    constexpr float speed = 10.0f;
    position += direction * speed * deltaTime;
}

struct DirectionKeys {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    Vec2D direction() const {
        return {static_cast<float>(right) - static_cast<float>(left),
            static_cast<float>(up) - static_cast<float>(down)};
    }
};

DWORD readConsoleKeys(HANDLE input, DirectionKeys& keys, bool& exitRequested) {
    if (input == INVALID_HANDLE_VALUE) return 0;

    INPUT_RECORD events[32];
    DWORD count = 0;
    if (!PeekConsoleInputW(input, events, _countof(events), &count)) return GetLastError();
    if (count == 0) return 0;
    if (!ReadConsoleInputW(input, events, count, &count)) return GetLastError();

    for (DWORD i = 0; i < count; ++i) {
        const auto& event = events[i];
        if (event.EventType == FOCUS_EVENT && !event.Event.FocusEvent.bSetFocus) {
            keys = {};
        }
        if (event.EventType != KEY_EVENT) continue;

        const auto& key = event.Event.KeyEvent;
        const bool pressed = key.bKeyDown != FALSE;
        switch (key.wVirtualKeyCode) {
        case VK_UP: keys.up = pressed; break;
        case VK_DOWN: keys.down = pressed; break;
        case VK_LEFT: keys.left = pressed; break;
        case VK_RIGHT: keys.right = pressed; break;
        case VK_ESCAPE: exitRequested = exitRequested || pressed; break;
        }
    }
    return 0;
}

void stopClient(ClientState& state) {
    {
        std::lock_guard<std::mutex> lock(positionMutex);
        state.running = false;
    }
    state.displayReady.notify_one();
}

int receiveRemotePositions(SOCKET sock, ClientState& state) {
    Clock::time_point lastReceived{};
    std::uint32_t lastSequence = 0;

    while (state.running) {
        const auto now = Clock::now();
        if (lastReceived != Clock::time_point{} && now - lastReceived >= PeerTimeout) {
            lastReceived = {};
            std::lock_guard<std::mutex> lock(positionMutex);
            state.hasRemotePosition = false;
            state.displayDirty = true;
            state.displayReady.notify_one();
        }

        PositionPacket packet{};
        const int bytes = recv(sock, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
        if (bytes == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) {
                Sleep(16);
                continue;
            }
            if (error == WSAEMSGSIZE) continue;
            return error;
        }

        PaddlePosition position{};
        if (bytes != static_cast<int>(sizeof(packet)) || !decodePosition(packet, position)) continue;

        std::lock_guard<std::mutex> lock(positionMutex);
        if (state.hasRemotePosition && !isNewerSequence(position.sequence, lastSequence)) continue;

        const bool changed = !state.hasRemotePosition || position.x != remoteBous.x ||
            position.y != remoteBous.y;
        remoteBous = {position.x, position.y};
        state.hasRemotePosition = true;
        lastSequence = position.sequence;
        lastReceived = now;
        if (changed) {
            state.displayDirty = true;
            state.displayReady.notify_one();
        }
    }
    return 0;
}

void printPositions(ClientState& state) {
    std::unique_lock<std::mutex> lock(positionMutex);
    while (true) {
        state.displayReady.wait_for(lock, std::chrono::milliseconds(100),
            [&] { return state.displayDirty || !state.running; });
        if (!state.displayDirty && !state.running) break;

        const Vec2D local = localBous;
        const Vec2D remote = remoteBous;
        const bool hasRemote = state.hasRemotePosition;
        state.displayDirty = false;
        lock.unlock();

        char queueLength[32] = "-";
        {
            std::unique_lock<std::mutex> queueLock(queueMutex, std::try_to_lock);
            if (queueLock.owns_lock()) {
                snprintf(queueLength, sizeof(queueLength), "%zu", pendingPositions.size());
            }
        }

        printf("\r[local] (%10.6g,%10.6g) [remote] (%10.6g,%10.6g) %-6s q=%-5s",
            local.x, local.y, remote.x, remote.y, hasRemote ? "" : "(wait)", queueLength);
        fflush(stdout);

        lock.lock();
    }

    lock.unlock();
    putchar('\n');
    fflush(stdout);
}

DWORD updateLocalPosition(HANDLE input, bool observe, ClientState& state) {
    DirectionKeys keys;
    auto previousUpdate = Clock::now();

    while (state.running) {
        bool exitRequested = false;
        const DWORD inputError = readConsoleKeys(input, keys, exitRequested);
        if (inputError != 0) return inputError;
        if (exitRequested) break;

        const auto now = Clock::now();
        const std::chrono::duration<float> deltaTime = now - previousUpdate;
        previousUpdate = now;

        const Vec2D direction = observe ? Vec2D{} : keys.direction();
        Vec2D position;
        bool changed;
        {
            std::lock_guard<std::mutex> lock(positionMutex);
            const Vec2D previous = localBous;
            updatePosition(localBous, direction, deltaTime.count());
            position = localBous;
            changed = position.x != previous.x || position.y != previous.y;
            if (changed) {
                state.displayDirty = true;
                state.displayReady.notify_one();
            }
        }

        if (changed) {
            const PositionPacket packet = encodePosition({0, position.x, position.y});
            std::lock_guard<std::mutex> lock(queueMutex);
            pendingPositions.push(packet);
        }

        Sleep(16);
    }

    return 0;
}

int sendLocalPositions(SOCKET sock, const ClientState& state) {
    PositionPacket lastSent = encodePosition({0, 0.0f, 0.0f});
    std::uint32_t nextSequence = 1;
    auto lastSend = Clock::now() - PositionRefreshInterval;

    while (state.running) {
        if (Clock::now() - lastSend >= SendInterval) {
            do {
                PositionPacket packet = lastSent;
                bool fromQueue;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    fromQueue = !pendingPositions.empty();
                    if (fromQueue) packet = pendingPositions.front();
                }
                if (!fromQueue && Clock::now() - lastSend < PositionRefreshInterval) break;

                packet.sequence = htonl(nextSequence);

                const int sentBytes = send(sock, reinterpret_cast<const char*>(&packet), sizeof(packet), 0);
                if (sentBytes == SOCKET_ERROR) {
                    const int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) break;
                    return error;
                }

                lastSent = packet;
                ++nextSequence;
                lastSend = Clock::now();
                if (!fromQueue) break;

                // Only this thread removes packets, so the front is still the one sent.
                std::lock_guard<std::mutex> lock(queueMutex);
                pendingPositions.pop();
            } while (state.running);
        }

        Sleep(16);
    }

    return 0;
}

int runClient(SOCKET sock, HANDLE input, bool observe) {
    ClientState state;
    int receiveError = 0;
    int sendError = 0;
    std::thread sendThread;
    std::thread receiveThread;
    std::thread outputThread;

    if (input == INVALID_HANDLE_VALUE) {
        puts("No console keyboard input.");
    } else {
        puts(observe ? "Local arrow-key input disabled. ESC to quit." : "Arrow keys to move. ESC to quit.");
    }
    fflush(stdout);

    try {
        receiveThread = std::thread([&] {
            receiveError = receiveRemotePositions(sock, state);
            stopClient(state);
        });
        outputThread = std::thread([&] { printPositions(state); });
        sendThread = std::thread([&] {
            sendError = sendLocalPositions(sock, state);
            stopClient(state);
        });
    } catch (const std::system_error& error) {
        stopClient(state);
        if (receiveThread.joinable()) receiveThread.join();
        if (outputThread.joinable()) outputThread.join();
        fprintf(stderr, "Could not start client threads (%s)\n", error.what());
        return 1;
    }

    const DWORD inputError = updateLocalPosition(input, observe, state);
    stopClient(state);
    sendThread.join();
    receiveThread.join();
    outputThread.join();

    if (inputError != 0) fprintf(stderr, "Console input failed (%lu)\n", inputError);
    if (sendError != 0) fprintf(stderr, "send() failed (%d)\n", sendError);
    if (receiveError != 0) fprintf(stderr, "recv() failed (%d)\n", receiveError);
    return inputError != 0 || sendError != 0 || receiveError != 0 ? 1 : 0;
}

int main(int argc, char** argv) {
    bool observe = false;
    const char* serverIp = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--observe") == 0) {
            observe = true;
        } else if (serverIp == nullptr) {
            serverIp = argv[i];
        } else {
            fprintf(stderr, "Usage: clienttest [server-ipv4] [--observe]\n");
            return 1;
        }
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(ServerPort);
    if (inet_pton(AF_INET, serverIp == nullptr ? "127.0.0.1" : serverIp, &server.sin_addr) != 1) {
        fprintf(stderr, "Usage: clienttest [server-ipv4] [--observe]\n");
        WSACleanup();
        return 1;
    }

    const SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed (%d)\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Connecting a UDP socket selects the server for both sending and receiving.
    u_long nonblocking = 1;
    int exitCode = 1;
    if (connect(sock, reinterpret_cast<const sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR ||
        ioctlsocket(sock, FIONBIO, &nonblocking) == SOCKET_ERROR) {
        fprintf(stderr, "UDP socket setup failed (%d)\n", WSAGetLastError());
    } else {
        const HANDLE input = CreateFileW(L"CONIN$", GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        exitCode = runClient(sock, input, observe);
        if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    }

    closesocket(sock);
    WSACleanup();
    return exitCode;
}
