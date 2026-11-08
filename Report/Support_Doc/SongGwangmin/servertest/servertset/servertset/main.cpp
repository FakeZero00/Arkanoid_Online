#include "Common.h"
#include "Vec2D.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

constexpr unsigned short SERVERPORT = 9000;

struct ReceivedPosition {
    Vec2DPacket position;
    sockaddr_in sender;
};

struct ConsoleState {
    std::mutex mutex;
    std::condition_variable ready;

    std::queue<ReceivedPosition> pending;

    bool finished = false;
    int receiveError = 0;
};

int receiveAndStore(SOCKET sock, ConsoleState& state) {
    while (true) {
        ReceivedPosition received{};
        int addrlen = sizeof(received.sender);
        constexpr int packetSize = sizeof(Vec2DPacket);

        const int bytesReceived = recvfrom(sock, reinterpret_cast<char*>(&received.position),
            packetSize, 0, reinterpret_cast<sockaddr*>(&received.sender), &addrlen);

        if (bytesReceived == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            if (error == WSAEMSGSIZE) continue;
            return error;
        }

        if (bytesReceived != packetSize) continue;

        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.pending.push(received);
        }
        state.ready.notify_one();
    }
}

void printToConsole(ConsoleState& state, Vec2D& PlayerBous) {
    std::unique_lock<std::mutex> lock(state.mutex);

    while (true) {
        state.ready.wait(lock, [&] { return !state.pending.empty() || state.finished; });

        if (state.pending.empty()) break;

        const ReceivedPosition received = state.pending.front();
        state.pending.pop();
        lock.unlock();

        PlayerBous.x = received.position.x;
        PlayerBous.y = received.position.y;

        char addr[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &received.sender.sin_addr, addr, sizeof(addr));
        printf("[UDP/%s:%d] (X: %f, Y: %f)\n", addr, ntohs(received.sender.sin_port),
            PlayerBous.x, PlayerBous.y);
        fflush(stdout);

        lock.lock();
    }

    const int receiveError = state.receiveError;
    lock.unlock();

    if (receiveError != 0) {
        WSASetLastError(receiveError);
        err_display("recvfrom()");
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    sockaddr_in serveraddr{};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(sock, reinterpret_cast<const sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");

    ConsoleState state;
    Vec2D PlayerBous;
    std::thread consoleThread([&] { printToConsole(state, PlayerBous); });

    const int receiveError = receiveAndStore(sock, state);

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.receiveError = receiveError;
        state.finished = true;
    }

    state.ready.notify_one();
    consoleThread.join();

    closesocket(sock);
    WSACleanup();
    return 0;
}
