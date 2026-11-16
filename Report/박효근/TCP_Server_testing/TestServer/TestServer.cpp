#include "../Protocol.h"
#include <ws2tcpip.h>
#include <cstdio>
#include <cstdlib>

constexpr uint16_t SERVER_PORT    = 9000;
constexpr uint16_t TIME_LIMIT_SEC = 180;
constexpr uint16_t COUNTDOWN_MS   = 3000;
constexpr uint8_t  MAX_STAGE      = 3;
constexpr uint16_t BLOCK_COUNT    = 66;    // 스테이지 1 기준 (6줄 x 11칸). 블록 번호 방식이 정해지면 수정

const char* EVENT_NAME[EVENT_COUNT] = {
    "BLOCK_HIT", "BLOCK_BROKEN", "ITEM_GET", "ITEM_END",
    "BALL_LOST", "RESPAWN", "STAGE_CLEAR", "STAGE_START",
};

struct Room;

// 클라이언트 1명 ------------------------------------------------------------------
struct Session {
    SOCKET sock = INVALID_SOCKET;
    int    id   = 0;
    Room*  room = nullptr;              // g_lobbyCs 안에서만 읽고 씀
    CRITICAL_SECTION sendLock;          // 같은 소켓에 두 스레드가 동시에 send 하지 않도록

    Session()  { InitializeCriticalSection(&sendLock); }
    ~Session() { DeleteCriticalSection(&sendLock); }

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

// 방 안의 플레이어 1명 상태 ---------------------------------------------------------
// 서버는 점수를 계산하지 않고, 클라이언트가 보낸 값을 검사한 뒤 받아 적기만 함
struct PlayerGameState {
    uint32_t score      = 0;       // 마지막으로 받은 총점
    uint8_t  stage      = 1;       // 지금 플레이 중인 스테이지
    bool     cleared    = false;   // 지금 스테이지를 클리어했는지 (STAGE_START를 받으면 다시 false)
    bool     finished   = false;   // 마지막 스테이지까지 클리어
    uint32_t lastTimeMs = 0;       // 마지막으로 받은 이벤트의 timeMs
};

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
    st.score      = p.score;
    st.lastTimeMs = p.timeMs;

    if (p.event == STAGE_CLEAR) {
        st.cleared = true;
        if (st.stage == MAX_STAGE) st.finished = true;
    } else if (p.event == STAGE_START) {
        st.stage   = p.arg8;
        st.cleared = false;
    }
}

// 방 (2명) ------------------------------------------------------------------------
enum class RoomState { Waiting, Playing, Ended };

struct Room {
    Session*        sessions[2] = {};
    PlayerGameState states[2];          // sessions[i]의 상태는 states[i]
    RoomState       state = RoomState::Waiting;
    CRITICAL_SECTION cs;

    Room()  { InitializeCriticalSection(&cs); }
    ~Room() { DeleteCriticalSection(&cs); }

    int IndexOf(Session* s) const { return sessions[0] == s ? 0 : (sessions[1] == s ? 1 : -1); }

    void StartGame()
    {
        uint32_t seed = (uint32_t)GetTickCount64() ^ ((uint32_t)rand() << 16);

        EnterCriticalSection(&cs);
        state = RoomState::Playing;
        Session* a = sessions[0];
        Session* b = sessions[1];
        LeaveCriticalSection(&cs);

        GameStartPacket pkt = MakeGameStart(seed, TIME_LIMIT_SEC, COUNTDOWN_MS);
        a->Send(pkt);
        b->Send(pkt);
        printf("[방] 게임 시작: 세션 %d vs 세션 %d (seed=%08X, 제한 %u초, 대기 %ums)\n",
               a->id, b->id, seed, TIME_LIMIT_SEC, COUNTDOWN_MS);
    }

    void OnGameEvent(Session* from, GameEventPacket pkt)
    {
        if (!IsValidEventType(pkt.event)) {
            printf("[방] 세션 %d: 잘못된 event 값 %u → 중계 안 함\n", from->id, pkt.event);
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
            printf("[방] 세션 %d: 게임 중이 아님 → 무시\n", from->id);
            return;
        }
        if (reject) {
            printf("[검사] 세션 %d: %s 거부 (%s) → 중계 안 함. 서버 기록 score=%u stage=%u, 받은 score=%u timeMs=%u arg8=%u arg16=%u\n",
                   from->id, EVENT_NAME[pkt.event], reject, after.score, after.stage,
                   pkt.score, pkt.timeMs, pkt.arg8, pkt.arg16);
            return;
        }

        printf("[중계] 세션 %d → 세션 %d : %s timeMs=%u arg8=%u arg16=%u arg32=%d score=%u\n",
               from->id, opp->id, EVENT_NAME[pkt.event], pkt.timeMs,
               pkt.arg8, pkt.arg16, pkt.arg32, pkt.score);
        printf("[상태] 세션 %d: score=%u stage=%u%s%s\n", from->id, after.score, after.stage,
               after.cleared ? " (클리어)" : "", after.finished ? " (모든 스테이지 완료)" : "");
        if (justFinished)
            printf("[상태] 세션 %d 모든 스테이지 클리어%s\n", from->id,
                   bothFinished ? " → 두 사람 모두 완료 (종료 판정은 11/26 일정)" : "");

        pkt.type = SC_OPP_EVENT;       // 본문은 그대로, type만 바꿈
        opp->Send(pkt);
    }

    void OnLeave(Session* s)
    {
        EnterCriticalSection(&cs);
        Session* opp = nullptr;
        uint32_t oppScore = 0, leftScore = 0;
        if (state == RoomState::Playing) {        // 확인과 변경을 같은 잠금 안에서 → 한 번만 종료
            state = RoomState::Ended;
            int idx = IndexOf(s);
            if (idx >= 0) {
                opp       = sessions[1 - idx];
                oppScore  = states[1 - idx].score;
                leftScore = states[idx].score;
            }
        }
        LeaveCriticalSection(&cs);

        if (opp) {
            opp->Send(MakeOppLeft());
            opp->Send(MakeGameEnd(RESULT_WIN, REASON_OPP_LEFT, oppScore, leftScore));  // 받는 사람 기준: 내 점수, 상대 점수
            printf("[방] 세션 %d 이탈 → 세션 %d 승리 처리 (점수 %u : %u)\n", s->id, opp->id, oppScore, leftScore);
        }
    }
};

// 매칭 (로비 파트 대신 간단하게) ----------------------------------------------------
CRITICAL_SECTION g_lobbyCs;
Session* g_waiting = nullptr;          // 상대를 기다리는 세션

Room* GetRoom(Session* s)
{
    EnterCriticalSection(&g_lobbyCs);
    Room* r = s->room;
    LeaveCriticalSection(&g_lobbyCs);
    return r;
}

// 2명이 모이면 방을 만들어 돌려줌. 아니면 nullptr
Room* Match(Session* s)
{
    Room* room = nullptr;
    EnterCriticalSection(&g_lobbyCs);
    if (g_waiting == nullptr) {
        g_waiting = s;
    } else {
        room = new Room();
        room->sessions[0] = g_waiting;
        room->sessions[1] = s;
        g_waiting->room = room;
        s->room = room;
        g_waiting = nullptr;
    }
    LeaveCriticalSection(&g_lobbyCs);
    return room;
}

void Unmatch(Session* s)
{
    EnterCriticalSection(&g_lobbyCs);
    if (g_waiting == s) g_waiting = nullptr;
    LeaveCriticalSection(&g_lobbyCs);
}

// 세션 수신 스레드 -------------------------------------------------------------------
DWORD WINAPI SessionThread(LPVOID arg)
{
    Session* s = (Session*)arg;
    char buf[MAX_PACKET_SIZE];
    int n;

    while ((n = RecvPacket(s->sock, buf, sizeof(buf))) > 0) {
        uint16_t type = PeekType(buf);

        if (!IsInGameTcp(type)) {
            printf("[세션 %d] 로비 패킷 type=%u (로비 파트 자리, 지금은 무시)\n", s->id, type);
            continue;
        }

        Room* room = GetRoom(s);
        if (!room) {
            printf("[세션 %d] 방이 없는데 인게임 패킷 type=%u → 무시\n", s->id, type);
            continue;
        }

        if (type == CS_GAME_EVENT) {
            GameEventPacket pkt;
            if (ReadTyped(buf, n, pkt)) room->OnGameEvent(s, pkt);
            else printf("[세션 %d] CS_GAME_EVENT 크기가 다름 (%d바이트) → 무시\n", s->id, n);
        } else {
            printf("[세션 %d] 클라이언트가 보낼 수 없는 type=%u → 무시\n", s->id, type);
        }
    }

    printf("[세션 %d] 연결 끊김 (%s)\n", s->id, n == 0 ? "정상 종료" : "오류 또는 비정상 패킷");

    Unmatch(s);
    if (Room* room = GetRoom(s)) room->OnLeave(s);
    s->Close();
    // 상대 스레드가 아직 이 Session을 가리킬 수 있어서 테스트 서버에서는 delete 하지 않음
    return 0;
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    setvbuf(stdout, nullptr, _IONBF, 0);       // 로그를 바로바로 출력 (파일로 저장할 때도 안 밀리게)
    InitializeCriticalSection(&g_lobbyCs);
    srand((unsigned)GetTickCount64());

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(SERVER_PORT);
    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
        listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("bind/listen 실패: %d\n", WSAGetLastError());
        return 1;
    }
    printf("테스트 서버 시작 (포트 %u). 클라이언트 2개를 접속시키세요.\n", SERVER_PORT);

    int nextId = 0;
    while (true) {
        sockaddr_in caddr{};
        int clen = sizeof(caddr);
        SOCKET cs = accept(listenSock, (sockaddr*)&caddr, &clen);
        if (cs == INVALID_SOCKET) {
            printf("accept 실패: %d\n", WSAGetLastError());
            continue;
        }

        Session* s = new Session();
        s->sock = cs;
        s->id   = nextId++;
        char ip[16];
        inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
        printf("[접속] 세션 %d (%s:%u)\n", s->id, ip, ntohs(caddr.sin_port));

        HANDLE h = CreateThread(nullptr, 0, SessionThread, s, 0, nullptr);
        if (h) CloseHandle(h);

        if (Room* room = Match(s)) room->StartGame();
        else printf("[매칭] 세션 %d 상대 대기 중\n", s->id);
    }
}
