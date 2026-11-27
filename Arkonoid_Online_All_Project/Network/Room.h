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
constexpr uint16_t SYNC_INTERVAL_SEC = 5;     // 남은 시간 알림(SC_TIME_SYNC) 간격
constexpr uint16_t GRACE_MS = 1000;           // 시간 종료 후 늦게 도착한 이벤트를 받아주는 유예 시간

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
    char playerName[2][32] = { {} };

public:
    explicit Room(Session* host);           // 방 개설: sessions[0] = 호스트, Waiting
    ~Room();                                // 타이머 스레드가 끝날 때까지 기다린 뒤 정리
    Room(const Room&) = delete;             // 복사 금지 (잠금 · 세션 포인터가 둘로 갈라짐)
    Room& operator=(const Room&) = delete;

    // 동작 -----------------------------------------------------------------
    bool Join(Session* guest);              // 참가: Waiting이고 빈자리가 있을 때만 성공
    void StartGame();                       // Waiting → Playing, 두 사람에게 SC_GAME_START, 타이머 스레드 시작
    void OnGameEvent(Session* from, GameEventPacket pkt);
    void OnLeave(Session* s);

    // 읽기 전용 -------------------------------------------------------------
    RoomState GetState() const;
    bool      IsFull() const;
    Session*  GetSession(int idx) const;    // idx: 0 = 호스트, 1 = 게스트. 범위 밖이면 nullptr
    uint32_t  GetScore(int idx) const;
    char*     GetRoomName() const;
	char* GetPlayerName(int idx) const;

private:
    int IndexOf(Session* s) const;          // 잠금을 잡은 상태에서만 호출
    void EndGame(EndReason reason);         // Playing/TimeUp → Ended, 점수 비교 후 두 사람에게 SC_GAME_END (한 번만)

    // 타이머 (방마다 스레드 1개) -------------------------------------------------
    static DWORD WINAPI TimerThread(LPVOID arg);
    void TimerLoop();                       // 1초마다 남은 시간 계산, 5초마다 SC_TIME_SYNC, 0초 → TimeUp → 유예 후 EndGame
    bool WaitUntil(ULONGLONG tick);         // 그 시각까지 대기. 중간에 멈추라는 신호가 오면 false

    Session*         sessions[2] = {};
    PlayerGameState  states[2];             // sessions[i]의 상태는 states[i]
    RoomState        state = RoomState::Waiting;
    ULONGLONG        startTick = 0;         // timeMs = 0 이 되는 시각 (GAME_START 전송 + 대기 시간)
    HANDLE           hTimer = nullptr;      // 타이머 스레드
    HANDLE           hStop = nullptr;       // 이탈 · 소멸 때 타이머를 바로 깨우는 신호
    mutable CRITICAL_SECTION cs;            // const 읽기 함수에서도 잠금을 잡을 수 있게 mutable
};
