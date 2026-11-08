#include "Common.h"
#include "Vec2D.h"

#include <Windows.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

constexpr char SERVERIP[] = "127.0.0.1";
constexpr unsigned short SERVERPORT = 9000;

struct PositionUpdate {
    Vec2D position;
    float deltaTime;
};

struct ClientState {
    std::mutex mutex;
    std::condition_variable sendReady;
    std::condition_variable consoleReady;

    std::queue<Vec2D> pendingPositions;
    PositionUpdate latest{};
    bool hasUpdate = false;

    bool inputFinished = false;
    bool sendFinished = false;
    int sendError = 0;
};

struct ClientThreadArgs {
    SOCKET sock;
    const sockaddr_in& serveraddr;
    Vec2D& position;
    ClientState& state;
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

DWORD WINAPI readInput(LPVOID parameter) {
    const auto& args = *static_cast<ClientThreadArgs*>(parameter);
    Vec2D& position = args.position;
    ClientState& state = args.state;

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

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        {
            std::unique_lock<std::mutex> lock(state.mutex);
            if (state.sendFinished) break;

            if (direction.x != 0.0f || direction.y != 0.0f) {
                updatePosition(position, direction, deltaTime);
                state.pendingPositions.push(position);
                state.latest = {position, deltaTime};
                state.hasUpdate = true;
                lock.unlock();

                state.sendReady.notify_one();
                state.consoleReady.notify_one();
            }
        }

        Sleep(16);
    }

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.inputFinished = true;
    }
    state.sendReady.notify_one();
    return 0;
}

DWORD WINAPI sendPositions(LPVOID parameter) {
    const auto& args = *static_cast<ClientThreadArgs*>(parameter);
    ClientState& state = args.state;

    std::unique_lock<std::mutex> lock(state.mutex);

    while (true) {
        state.sendReady.wait(lock, [&] { return !state.pendingPositions.empty() || state.inputFinished; });

        if (state.pendingPositions.empty()) break;

        const Vec2D position = state.pendingPositions.front();
        state.pendingPositions.pop();
        lock.unlock();

        const int sendError = sendPosition(args.sock, args.serveraddr, position);

        lock.lock();
        if (sendError != 0) {
            state.sendError = sendError;
            break;
        }
    }

    state.sendFinished = true;
    lock.unlock();
    state.consoleReady.notify_one();
    return 0;
}

DWORD WINAPI printToConsole(LPVOID parameter) {
    const auto& args = *static_cast<ClientThreadArgs*>(parameter);
    ClientState& state = args.state;

    puts("=========================================\n"
        " deltaTime 기반 2D Vector 이동 테스트\n"
        " 방향키(UP, DOWN, LEFT, RIGHT)로 이동\n"
        " 종료하려면 ESC 키를 누르세요.\n"
        "=========================================\n");
    fflush(stdout);

    std::unique_lock<std::mutex> lock(state.mutex);

    while (true) {
        state.consoleReady.wait(lock, [&] { return state.hasUpdate || state.sendFinished; });

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

    return 0;
}

int runClientThreads(SOCKET sock, const sockaddr_in& serveraddr, Vec2D& position) {
    ClientState state;
    ClientThreadArgs args{sock, serveraddr, position, state};

    // Start input last so a creation failure can stop the waiting threads.
    const LPTHREAD_START_ROUTINE workers[]{sendPositions, printToConsole, readInput};
    HANDLE threads[_countof(workers)]{};
    DWORD started = 0;

    while (started < _countof(workers)) {
        threads[started] = CreateThread(nullptr, 0, workers[started], &args, 0, nullptr);
        if (threads[started] == nullptr) break;
        ++started;
    }

    if (started < _countof(workers)) {
        const DWORD error = GetLastError();
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.inputFinished = true;
        }

        state.sendReady.notify_one();
        fprintf(stderr, "[CreateThread()] %lu\n", error);
    }

    for (DWORD i = 0; i < started; ++i) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    return started == _countof(workers) ? 0 : 1;
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
    const int exitCode = runClientThreads(sock, serveraddr, position);

    closesocket(sock);
    WSACleanup();
    return exitCode;
}
