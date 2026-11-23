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

// 방 (2명) ------------------------------------------------------------------------
// 속성은 모두 private. 밖에서는 "하는 일" 단위 함수와 읽기 전용 함수만 사용
// 읽기 함수는 잠금을 잡고 값을 복사해서 돌려줌 → 받은 값은 "그 순간의 값"
// 상태를 바꾸는 판단(확인 + 변경)은 반드시 Room 안의 함수에서 잠금 하나로 처리
class Room {
private:
    char roomName[32] = {};

public:
    explicit Room(Session* host);           // 방 개설: sessions[0] = 호스트, Waiting
    ~Room();
    Room(const Room&) = delete;             // 복사 금지 (잠금 · 세션 포인터가 둘로 갈라짐)
    Room& operator=(const Room&) = delete;

    // 동작 -----------------------------------------------------------------
    bool Join(Session* guest);              // 참가: Waiting이고 빈자리가 있을 때만 성공
    void StartGame();                       // Waiting → Playing, 두 사람에게 SC_GAME_START
    void OnGameEvent(Session* from, GameEventPacket pkt);
    void OnLeave(Session* s);

    // 읽기 전용 -------------------------------------------------------------
    RoomState GetState() const;
    bool      IsFull() const;
    Session*  GetSession(int idx) const;    // idx: 0 = 호스트, 1 = 게스트. 범위 밖이면 nullptr
    uint32_t  GetScore(int idx) const;
    char*     GetRoomName() const;

private:
    int IndexOf(Session* s) const;          // 잠금을 잡은 상태에서만 호출

    Session*         sessions[2] = {};
    PlayerGameState  states[2];             // sessions[i]의 상태는 states[i]
    RoomState        state = RoomState::Waiting;
    mutable CRITICAL_SECTION cs;            // const 읽기 함수에서도 잠금을 잡을 수 있게 mutable
};
