#include "Room.h"

// 이벤트 검사 · 반영 (Room.cpp 안에서만 사용) -----------------------------------------
namespace {

// 이벤트가 말이 되는지 검사. 정상이면 nullptr, 이상하면 이유 문자열
const char* ValidateEvent(const PlayerGameState& st, const GameEventPacket& p)
{
    if (p.score < st.score)       return "점수가 줄어듦";
    if (p.timeMs < st.lastTimeMs) return "timeMs가 이전보다 작음";

    switch (p.event) {
    case BLOCK_HIT:
    case BLOCK_BROKEN:
        if (p.arg16 >= BLOCK_COUNT) return "블록 인덱스 범위 밖";
        break;
    case STAGE_CLEAR:
        if (p.arg8 != st.stage) return "지금 스테이지와 다른 번호를 클리어";
        if (st.cleared)         return "이미 클리어한 스테이지";
        break;
    case STAGE_START:
        if (!st.cleared)              return "지금 스테이지를 클리어하지 않고 다음 스테이지 시작";
        if (p.arg8 != st.stage + 1)   return "스테이지 순서가 맞지 않음";
        if (p.arg8 > MAX_STAGE)       return "스테이지 범위 밖";
        break;
    default:
        break;
    }
    return nullptr;
}

// 검사를 통과한 이벤트를 상태에 반영
void ApplyEvent(PlayerGameState& st, const GameEventPacket& p)
{
    st.score = p.score;
    st.lastTimeMs = p.timeMs;

    if (p.event == STAGE_CLEAR) {
        st.cleared = true;
        if (st.stage == MAX_STAGE) st.finished = true;
    }
    else if (p.event == STAGE_START) {
        st.stage = p.arg8;
        st.cleared = false;
    }
}

} // namespace

// 생성 · 소멸 ----------------------------------------------------------------------
Room::Room(Session* host)
{
    InitializeCriticalSection(&cs);
    sessions[0] = host;
}

Room::~Room()
{
    DeleteCriticalSection(&cs);
}

// 읽기 전용 ------------------------------------------------------------------------
RoomState Room::GetState() const
{
    EnterCriticalSection(&cs);
    RoomState s = state;
    LeaveCriticalSection(&cs);
    return s;
}

bool Room::IsFull() const
{
    EnterCriticalSection(&cs);
    bool full = (sessions[0] != nullptr && sessions[1] != nullptr);
    LeaveCriticalSection(&cs);
    return full;
}

Session* Room::GetSession(int idx) const
{
    if (idx < 0 || idx > 1) return nullptr;
    EnterCriticalSection(&cs);
    Session* s = sessions[idx];
    LeaveCriticalSection(&cs);
    return s;
}

uint32_t Room::GetScore(int idx) const
{
    if (idx < 0 || idx > 1) return 0;
    EnterCriticalSection(&cs);
    uint32_t score = states[idx].score;
    LeaveCriticalSection(&cs);
    return score;
}

int Room::IndexOf(Session* s) const
{
    return sessions[0] == s ? 0 : (sessions[1] == s ? 1 : -1);
}

// 동작 -----------------------------------------------------------------------------
bool Room::Join(Session* guest)
{
    EnterCriticalSection(&cs);
    bool ok = (state == RoomState::Waiting && sessions[1] == nullptr && guest != sessions[0]);
    if (ok) sessions[1] = guest;
    LeaveCriticalSection(&cs);
    return ok;
}

void Room::StartGame()
{
    uint32_t seed = (uint32_t)GetTickCount64() ^ ((uint32_t)rand() << 16);

    // 확인과 변경을 같은 잠금 안에서 → 두 번 시작되지 않음
    EnterCriticalSection(&cs);
    bool ok = (state == RoomState::Waiting && sessions[0] && sessions[1]);
    if (ok) state = RoomState::Playing;
    Session* a = sessions[0];
    Session* b = sessions[1];
    LeaveCriticalSection(&cs);

    if (!ok) {
        printf("[방] 게임 시작 실패: 대기 상태가 아니거나 두 명이 모이지 않음\n");
        return;
    }

    GameStartPacket pkt = MakeGameStart(seed, TIME_LIMIT_SEC, COUNTDOWN_MS);
    a->Send(pkt);
    b->Send(pkt);
    printf("[방] 게임 시작: 세션 %d vs 세션 %d (seed=%08X, 제한 %u초, 대기 %ums)\n",
        a->GetId(), b->GetId(), seed, TIME_LIMIT_SEC, COUNTDOWN_MS);
}

void Room::OnGameEvent(Session* from, GameEventPacket pkt)
{
    if (!IsValidEventType(pkt.event)) {
        printf("[방] 세션 %d: 잘못된 event 값 %u → 중계 안 함\n", from->GetId(), pkt.event);
        return;
    }

    // 잠금 안에서는 검사 · 갱신 · 복사만 하고, send와 printf는 잠금 밖에서
    EnterCriticalSection(&cs);
    Session*        opp = nullptr;
    const char*     reject = nullptr;
    PlayerGameState after{};
    bool            bothFinished = false, justFinished = false;
    int idx = IndexOf(from);
    if (state == RoomState::Playing && idx >= 0) {
        PlayerGameState& st = states[idx];
        reject = ValidateEvent(st, pkt);
        if (!reject) {
            bool wasFinished = st.finished;
            ApplyEvent(st, pkt);
            justFinished = !wasFinished && st.finished;
            bothFinished = states[0].finished && states[1].finished;
            opp = sessions[1 - idx];
        }
        after = st;
    }
    LeaveCriticalSection(&cs);

    if (idx < 0 || (!opp && !reject)) {
        printf("[방] 세션 %d: 게임 중이 아님 → 무시\n", from->GetId());
        return;
    }
    if (reject) {
        printf("[검사] 세션 %d: %s 거부 (%s) → 중계 안 함. 서버 기록 score=%u stage=%u, 받은 score=%u timeMs=%u arg8=%u arg16=%u\n",
            from->GetId(), EVENT_NAME[pkt.event], reject, after.score, after.stage,
            pkt.score, pkt.timeMs, pkt.arg8, pkt.arg16);
        return;
    }

    printf("[중계] 세션 %d → 세션 %d : %s timeMs=%u arg8=%u arg16=%u arg32=%d score=%u\n",
        from->GetId(), opp->GetId(), EVENT_NAME[pkt.event], pkt.timeMs,
        pkt.arg8, pkt.arg16, pkt.arg32, pkt.score);
    printf("[상태] 세션 %d: score=%u stage=%u%s%s\n", from->GetId(), after.score, after.stage,
        after.cleared ? " (클리어)" : "", after.finished ? " (모든 스테이지 완료)" : "");
    if (justFinished)
        printf("[상태] 세션 %d 모든 스테이지 클리어%s\n", from->GetId(),
            bothFinished ? " → 두 사람 모두 완료 (종료 판정은 11/26 일정)" : "");

    pkt.type = SC_OPP_EVENT;       // 본문은 그대로, type만 바꿈
    opp->Send(pkt);
}

void Room::OnLeave(Session* s)
{
    EnterCriticalSection(&cs);
    Session* opp = nullptr;
    uint32_t oppScore = 0, leftScore = 0;
    int idx = IndexOf(s);
    if (state == RoomState::Playing) {        // 확인과 변경을 같은 잠금 안에서 → 한 번만 종료
        state = RoomState::Ended;
        if (idx >= 0) {
            opp = sessions[1 - idx];
            oppScore = states[1 - idx].score;
            leftScore = states[idx].score;
        }
    }
    else if (state == RoomState::Waiting && idx == 1) {
        sessions[1] = nullptr;                // 시작 전에 게스트가 나가면 자리만 비움
    }
    LeaveCriticalSection(&cs);

    if (opp) {
        opp->Send(MakeOppLeft());
        opp->Send(MakeGameEnd(RESULT_WIN, REASON_OPP_LEFT, oppScore, leftScore));  // 받는 사람 기준: 내 점수, 상대 점수
        printf("[방] 세션 %d 이탈 → 세션 %d 승리 처리 (점수 %u : %u)\n", s->GetId(), opp->GetId(), oppScore, leftScore);
    }
}
