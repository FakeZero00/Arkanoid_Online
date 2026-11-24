#include "Room.h"

// 이벤트 검사 · 반영 (Room.cpp 안에서만 사용) -----------------------------------------
namespace {

// 이벤트가 말이 되는지 검사. 정상이면 nullptr, 이상하면 이유 문자열
const char* ValidateEvent(const PlayerGameState& st, const GameEventPacket& p)
{
    if (p.score < st.score)       return "점수가 줄어듦";
    if (p.timeMs < st.lastTimeMs) return "timeMs가 이전보다 작음";
    if (p.timeMs > TIME_LIMIT_SEC * 1000u) return "제한시간이 지난 뒤의 이벤트";

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
    hStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);     // 수동 리셋: 한 번 켜면 계속 켜짐
    sessions[0] = host;
}

Room::~Room()
{
    // 타이머 스레드가 this를 쓰고 있을 수 있으므로, 깨워서 끝날 때까지 기다린 뒤 정리
    // (타이머 스레드 안에서 delete 하면 안 됨)
    if (hStop) SetEvent(hStop);
    if (hTimer) {
        WaitForSingleObject(hTimer, INFINITE);
        CloseHandle(hTimer);
    }
    if (hStop) CloseHandle(hStop);
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

char* Room::GetRoomName() const
{
	return (char*)roomName;
}

char* Room::GetPlayerName(int idx) const
{
	if (idx < 0 || idx > 1) return nullptr;
	return (char*)playerName[idx];
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
    if (ok) {
        state = RoomState::Playing;
        startTick = GetTickCount64() + COUNTDOWN_MS;
    }
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

    // 방마다 타이머 스레드 1개. 위의 ok 확인 덕분에 한 번만 만들어짐
    hTimer = CreateThread(nullptr, 0, TimerThread, this, 0, nullptr);
    if (!hTimer) printf("[방] 타이머 스레드 생성 실패: %lu\n", GetLastError());
}

// 타이머 ---------------------------------------------------------------------------
DWORD WINAPI Room::TimerThread(LPVOID arg)
{
    static_cast<Room*>(arg)->TimerLoop();
    return 0;
}

bool Room::WaitUntil(ULONGLONG tick)
{
    ULONGLONG now = GetTickCount64();
    DWORD ms = (tick > now) ? (DWORD)(tick - now) : 0;
    return WaitForSingleObject(hStop, ms) == WAIT_TIMEOUT;     // 신호가 오면 바로 깨어남
}

// 1초씩 세되 시작 시각 기준으로 기다려서 오차가 쌓이지 않음.
// 상태 확인과 변경은 같은 잠금 안에서, send와 printf는 잠금 밖에서
void Room::TimerLoop()
{
    for (uint16_t sec = 0; sec <= TIME_LIMIT_SEC; ++sec) {
        if (!WaitUntil(startTick + sec * 1000ULL)) {
            printf("[타이머] 게임이 끝나서 타이머 종료\n");
            return;
        }
        uint16_t remain = (uint16_t)(TIME_LIMIT_SEC - sec);

        EnterCriticalSection(&cs);
        bool running = (state == RoomState::Playing);
        if (running && remain == 0) state = RoomState::TimeUp;
        Session* a = sessions[0];
        Session* b = sessions[1];
        LeaveCriticalSection(&cs);

        if (!running) {
            printf("[타이머] 게임이 이미 끝나서 타이머 종료 (남은 시간 %u초 시점)\n", remain);
            return;
        }
        if (remain % SYNC_INTERVAL_SEC == 0) {
            TimeSyncPacket pkt = MakeTimeSync(remain);
            a->Send(pkt);
            b->Send(pkt);
            printf("[타이머] 남은 시간 %u초 → 두 클라이언트에 전송\n", remain);
        }
    }
    printf("[타이머] 시간 종료 → %ums 동안 제한시간 안의 이벤트만 받음\n", GRACE_MS);

    if (!WaitUntil(startTick + TIME_LIMIT_SEC * 1000ULL + GRACE_MS)) return;

    EnterCriticalSection(&cs);
    bool ended = (state == RoomState::TimeUp);
    if (ended) state = RoomState::Ended;
    uint32_t s0 = states[0].score, s1 = states[1].score;
    int id0 = sessions[0]->GetId(), id1 = sessions[1]->GetId();
    LeaveCriticalSection(&cs);

    if (ended)
        printf("[타이머] 유예 끝 → 게임 종료. 세션 %d %u점 : 세션 %d %u점 (승패 판정 · 결과 전송은 11/26 EndGame)\n",
            id0, s0, id1, s1);
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
    bool            inGrace = (state == RoomState::TimeUp);   // 유예 중: 검사에서 timeMs가 제한시간 안인 것만 통과
    int idx = IndexOf(from);
    if ((state == RoomState::Playing || inGrace) && idx >= 0) {
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

    printf("[중계] 세션 %d → 세션 %d : %s timeMs=%u arg8=%u arg16=%u arg32=%d score=%u%s\n",
        from->GetId(), opp->GetId(), EVENT_NAME[pkt.event], pkt.timeMs,
        pkt.arg8, pkt.arg16, pkt.arg32, pkt.score,
        inGrace ? " (유예 시간 중 도착, 제한시간 안의 이벤트라 받음)" : "");
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
    bool stopTimer = false;
    if (state == RoomState::Playing || state == RoomState::TimeUp) {  // 확인과 변경을 같은 잠금 안에서 → 한 번만 종료
        state = RoomState::Ended;
        stopTimer = true;
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

    if (stopTimer) SetEvent(hStop);           // 타이머가 다음 1초까지 기다리지 않고 바로 끝나게

    if (opp) {
        opp->Send(MakeOppLeft());
        opp->Send(MakeGameEnd(RESULT_WIN, REASON_OPP_LEFT, oppScore, leftScore));  // 받는 사람 기준: 내 점수, 상대 점수
        printf("[방] 세션 %d 이탈 → 세션 %d 승리 처리 (점수 %u : %u)\n", s->GetId(), opp->GetId(), oppScore, leftScore);
    }
}

////////////////////패킷용 함수//////////////////////////
//방 정보 송신
bool SendRoomList(SOCKET sock, std::vector<Room*> rooms)
{
    RoomListPacket p{};
    p.size = sizeof(RoomListPacket);
    p.type = SC_ROOM_LIST;
    p.roomCount = static_cast<uint8_t>(rooms.size());
    for (int i = 0; i < p.roomCount; ++i) {
        strncpy_s(p.roomName[i], rooms[i]->GetRoomName(), sizeof(p.roomName[i]) - 1);
        p.roomName[i][sizeof(p.roomName[i]) - 1] = '\0';
    }

    return SendTyped(sock, p);
}