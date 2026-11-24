#include "Network.h"        // winsock2.h를 다른 헤더보다 먼저
#include "Protocol.h"
#include "GameNet.h"
#include <atomic>

extern SOCKET sock;         // main.cpp의 로비 · 인게임 공용 TCP 소켓

namespace {

// 수신 스레드가 쓰고 메인 스레드가 읽으므로 atomic
std::atomic<unsigned long long> g_startTick{ 0 };   // timeMs = 0 이 되는 시각. 0 = 시작 전
std::atomic<uint32_t>           g_limitMs{ 0 };
std::atomic<bool>               g_ended{ false };
std::atomic<uint32_t>           g_score{ 0 };

// 같은 소켓에 두 스레드가 동시에 send 하지 않도록 (게임 중 송신끼리)
struct SendLock {
    CRITICAL_SECTION cs;
    SendLock()  { InitializeCriticalSection(&cs); }
    ~SendLock() { DeleteCriticalSection(&cs); }
} g_sendLock;

bool SendEvent(EventType e, uint8_t a8 = 0, uint16_t a16 = 0, int32_t a32 = 0)
{
    if (!GameNet::IsPlaying()) return false;    // 혼자 테스트 중이거나 끝난 뒤

    GameEventPacket pkt = MakeGameEvent(e, GameNet::GetTimeMs(), g_score, a8, a16, a32);

    EnterCriticalSection(&g_sendLock.cs);
    bool ok = SendTyped(sock, pkt);
    LeaveCriticalSection(&g_sendLock.cs);

    if (!ok) printf("[GameNet] 이벤트 %u 전송 실패: %d\n", (unsigned)e, WSAGetLastError());
    return ok;
}

} // namespace

namespace GameNet {

void OnGameStart(uint16_t timeLimitSec, uint16_t countdownMs)
{
    g_score   = 0;
    g_ended   = false;
    g_limitMs = timeLimitSec * 1000u;
    g_startTick = GetTickCount64() + countdownMs;   // 마지막에 써서, 다른 값이 준비된 뒤 시작으로 보이게
}

void OnGameEnd()
{
    g_ended = true;
}

bool IsPlaying()
{
    return g_startTick != 0 && !g_ended;
}

uint32_t GetTimeMs()
{
    unsigned long long start = g_startTick, now = GetTickCount64();
    if (start == 0 || now < start) return 0;    // 시작 전 · 대기 시간 중
    return (uint32_t)(now - start);
}

uint32_t GetScore()
{
    return g_score;
}

bool SendBlockHit(uint16_t blockIdx, int remainHp)
{
    return SendEvent(BLOCK_HIT, 0, blockIdx, remainHp);
}

bool SendBlockBroken(uint16_t blockIdx, int points)
{
    if (IsPlaying()) g_score += points;         // 서버는 "반영한 뒤의 총점"을 받음
    return SendEvent(BLOCK_BROKEN, 0, blockIdx, points);
}

bool SendItemGet(uint8_t itemType)  { return SendEvent(ITEM_GET, itemType); }
bool SendItemEnd(uint8_t itemType)  { return SendEvent(ITEM_END, itemType); }
bool SendBallLost(int respawnMs)    { return SendEvent(BALL_LOST, 0, 0, respawnMs); }
bool SendRespawn()                  { return SendEvent(RESPAWN); }

bool SendStageClear(uint8_t stage, uint16_t ballsLeft, int bonus)
{
    if (IsPlaying()) g_score += bonus;
    return SendEvent(STAGE_CLEAR, stage, ballsLeft, bonus);
}

bool SendStageStart(uint8_t stage)  { return SendEvent(STAGE_START, stage); }

} // namespace GameNet
