#include "Common.h"
#include "Vec2D.h"

#include <Windows.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

constexpr char SERVERIP[] = "127.0.0.1";
constexpr unsigned short SERVERPORT = 9000;

struct PositionUpdate {
    Vec2DPacket position;
    float deltaTime;
};

struct ConsoleState {
    std::mutex mutex;
    std::condition_variable ready;

    PositionUpdate latest{};
    bool hasUpdate = false;

    bool finished = false;
    int sendError = 0;
};

void updatePosition(Vec2D& position, const Vec2D& direction, float deltaTime) {
    constexpr float speed = 10.0f;
    position += direction * speed * deltaTime;
}

int sendPosition(SOCKET sock, const sockaddr_in& serveraddr, const Vec2D& position) {
    const Vec2DPacket packet{position.x, position.y};

    if (sendto(sock, reinterpret_cast<const char*>(&packet), sizeof(packet), 0,
        reinterpret_cast<const sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    return 0;
}

int readInputAndSend(SOCKET sock, const sockaddr_in& serveraddr,
    Vec2D& position, ConsoleState& state) {
    auto previousTime = std::chrono::steady_clock::now();

    while (true) {
        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsedTime = currentTime - previousTime;
        const float deltaTime = elapsedTime.count();
        previousTime = currentTime;

        Vec2D direction(0.0f, 0.0f);

        if (GetAsyncKeyState(VK_UP) & 0x8000)    direction.y += 1.0f;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)  direction.y -= 1.0f;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  direction.x -= 1.0f;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) direction.x += 1.0f;

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return 0;

        if (direction.x != 0.0f || direction.y != 0.0f) {
            updatePosition(position, direction, deltaTime);

            {
                std::lock_guard<std::mutex> lock(state.mutex);
                state.latest = {{position.x, position.y}, deltaTime};
                state.hasUpdate = true;
            }
            state.ready.notify_one();

            const int sendError = sendPosition(sock, serveraddr, position);
            if (sendError != 0) return sendError;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void printToConsole(ConsoleState& state) {
    puts("=========================================\n"
        " deltaTime 기반 2D Vector 이동 테스트\n"
        " 방향키(UP, DOWN, LEFT, RIGHT)로 이동\n"
        " 종료하려면 ESC 키를 누르세요.\n"
        "=========================================\n");
    fflush(stdout);

    std::unique_lock<std::mutex> lock(state.mutex);

    while (true) {
        state.ready.wait(lock, [&] { return state.hasUpdate || state.finished; });

        if (!state.hasUpdate) break;

        const PositionUpdate update = state.latest;
        state.hasUpdate = false;
        lock.unlock();

        printf("\r[위치] X: %f,\tY: %f\t(dt: %f)",
            update.position.x, update.position.y, update.deltaTime);
        fflush(stdout);

        lock.lock();
    }

    const int sendError = state.sendError;
    lock.unlock();

    if (sendError != 0) {
        putchar('\n');
        WSASetLastError(sendError);
        err_display("sendto()");
    } else {
        puts("\n프로그램을 종료합니다.");
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
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    Vec2D position;
    ConsoleState state;
    std::thread consoleThread([&] { printToConsole(state); });

    const int sendError = readInputAndSend(sock, serveraddr, position, state);

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.sendError = sendError;
        state.finished = true;
    }

    state.ready.notify_one();
    consoleThread.join();

    closesocket(sock);
    WSACleanup();
    return 0;
}
