#pragma once
#include "Network.h"
#include "Protocol.h"
#include "Session.h"
#include "PlayerGameState.h"
#include "RoomState.h"

constexpr uint16_t SERVER_PORT = 9000;
constexpr uint16_t TIME_LIMIT_SEC = 180;
constexpr uint16_t COUNTDOWN_MS = 3000;
constexpr uint8_t  MAX_STAGE = 3;
constexpr uint16_t BLOCK_COUNT = 66;    // 스테이지 1 기준 (6줄 x 11칸). 블록 번호 방식이 정해지면 수정

inline constexpr const char* EVENT_NAME[EVENT_COUNT] = {
    "BLOCK_HIT", "BLOCK_BROKEN", "ITEM_GET", "ITEM_END",
    "BALL_LOST", "RESPAWN", "STAGE_CLEAR", "STAGE_START",
};


class Room {
    Session* sessions[2] = {};
    PlayerGameState states[2];          // sessions[i]의 상태는 states[i]
    RoomState       state = RoomState::Waiting;
    CRITICAL_SECTION cs;

    Room() { InitializeCriticalSection(&cs); }
    ~Room() { DeleteCriticalSection(&cs); }

    int IndexOf(Session* s) const { return sessions[0] == s ? 0 : (sessions[1] == s ? 1 : -1); }

    void StartGame();
    void OnGameEvent(Session* from, GameEventPacket pkt);
    void OnLeave(Session* s);
};