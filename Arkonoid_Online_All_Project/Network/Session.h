#pragma once
#include "Network.h"
#include <string>

class Room;

class Session {
private:
    SOCKET sock = INVALID_SOCKET;
    int    id = 0;
    std::string playerName;
    Room* room = nullptr;              // g_lobbyCs 안에서만 읽고 씀
    CRITICAL_SECTION sendLock;          // 같은 소켓에 두 스레드가 동시에 send 하지 않도록

public:
    Session() { InitializeCriticalSection(&sendLock); }
    ~Session() { DeleteCriticalSection(&sendLock); }

    // 게터 함수
	SOCKET GetSocket() const { return sock; }
	int GetId() const { return id; }
	std::string GetPlayerName() const { return playerName; }

    //세터 함수
    void SetSocket(SOCKET s) { sock = s; }
    void SetId(int i) { id = i; }
    void SetPlayerName(const std::string& n) { playerName = n; }

    template <typename T>
    bool Send(const T& pkt)
    {
        EnterCriticalSection(&sendLock);
        bool ok = (sock != INVALID_SOCKET) && SendTyped(sock, pkt);
        LeaveCriticalSection(&sendLock);
        return ok;
    }

    // 끊긴 소켓 핸들이 다른 클라이언트 소켓에 재사용되기 전에 막아둠
    void Close()
    {
        EnterCriticalSection(&sendLock);
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        LeaveCriticalSection(&sendLock);
    }
};