#pragma once
// ----------------------------------------------------------------------------------
//  Protocol.h  -  인게임 TCP 패킷 정의와 송수신 함수
//
//  - 로비와 같은 TCP 소켓을 쓰고, 헤더의 type 번호 대역으로 구분
//      1 ~ 49    : 로비 TCP
//      50 ~ 99   : 인게임 UDP
//      100 ~ 149 : 인게임 TCP
//  - 구조체는 호스트 바이트 순서로 채우고, Send 함수가 보내기 직전에
//  네트워크 순서로 바꿈. 받은 뒤에는 ToHost()로 되돌려서 사용
// ----------------------------------------------------------------------------------
#include <winsock2.h>
#include <cstdint>
#pragma comment(lib, "ws2_32")

constexpr uint16_t IN_GAME_TCP_BEGIN = 100;
constexpr uint16_t IN_GAME_TCP_END   = 150;
constexpr int      MAX_PACKET_SIZE   = 512;      // RecvPacket 버퍼 크기

enum InGameTcpType : uint16_t {
    CS_GAME_EVENT = IN_GAME_TCP_BEGIN,   // 클라이언트 → 서버 : 내 이벤트
    SC_OPP_EVENT,                        // 서버 → 클라이언트 : 상대 이벤트 (본문은 CS_GAME_EVENT와 같음)
    SC_TIME_SYNC,                        // 서버 → 클라이언트 : 남은 시간
    SC_GAME_END,                         // 서버 → 클라이언트 : 게임 결과
    SC_OPP_LEFT,                         // 서버 → 클라이언트 : 상대 연결 끊김
    SC_GAME_START,                       // 서버 → 클라이언트 : 게임 시작 (시드, 제한시간, 대기 시간)
};

enum EventType : uint8_t {
    BLOCK_HIT,      // arg16 블록 인덱스, arg32 남은 내구도
    BLOCK_BROKEN,   // arg16 블록 인덱스, arg32 얻은 점수
    ITEM_GET,       // arg8  아이템 종류
    ITEM_END,       // arg8  아이템 종류
    BALL_LOST,      // arg32 리스폰 대기 시간(ms)
    RESPAWN,        // 인자 없음
    STAGE_CLEAR,    // arg8  스테이지 번호, arg16 남은 공 수, arg32 보너스 점수
    STAGE_START,    // arg8  스테이지 번호
    EVENT_COUNT     // 개수 (값 검사용)
};

enum GameResult : uint8_t { RESULT_LOSE, RESULT_WIN, RESULT_DRAW };                   // 받는 사람 기준
enum EndReason  : uint8_t { REASON_TIME_OVER, REASON_ALL_CLEAR, REASON_OPP_LEFT };

inline bool IsInGameTcp(uint16_t type) { return type >= IN_GAME_TCP_BEGIN && type < IN_GAME_TCP_END; }

// 구조체-----------------------------------------------------------------------------------
#pragma pack(push, 1)
struct PacketHeader {
    uint16_t size;                 // 헤더 포함 전체 바이트 수
    uint16_t type;                 // 위 번호 대역
};

struct GameEventPacket : PacketHeader {  // CS_GAME_EVENT, SC_OPP_EVENT 공용
    uint32_t timeMs;               // 게임 시작 후 경과 시간 (제한시간 판정 · 디버깅용)
    uint8_t  event;                // EventType
    uint8_t  arg8;
    uint16_t arg16;
    int32_t  arg32;
    uint32_t score;                // 이 이벤트를 반영한 뒤의 내 총점
};

struct TimeSyncPacket : PacketHeader {
    uint16_t remainSec;            // 남은 시간(초)
};

struct GameEndPacket : PacketHeader {
    uint8_t  result;               // GameResult
    uint8_t  reason;               // EndReason
    uint32_t myScore;
    uint32_t oppScore;
};

struct GameStartPacket : PacketHeader {
    uint32_t seed;                 // 아이템 배치용 랜덤 시드 (두 사람 같은 값)
    uint16_t timeLimitSec;         // 제한시간(초)
    uint16_t countdownMs;          // 받은 뒤 이만큼 기다렸다가 timeMs = 0으로 시작
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader)    == 4,  "PacketHeader 크기 확인");
static_assert(sizeof(GameEventPacket) == 20, "GameEventPacket 크기 확인");
static_assert(sizeof(TimeSyncPacket)  == 6,  "TimeSyncPacket 크기 확인");
static_assert(sizeof(GameEndPacket)   == 14, "GameEndPacket 크기 확인");
static_assert(sizeof(GameStartPacket) == 12, "GameStartPacket 크기 확인");

// 패킷 ---------------------------------------------------------------------------------
inline GameEventPacket MakeGameEvent(EventType e, uint32_t timeMs, uint32_t score,
                                     uint8_t a8 = 0, uint16_t a16 = 0, int32_t a32 = 0)
{
    GameEventPacket p{};
    p.size   = sizeof(GameEventPacket);
    p.type   = CS_GAME_EVENT;
    p.timeMs = timeMs;
    p.event  = e;
    p.arg8   = a8;
    p.arg16  = a16;
    p.arg32  = a32;
    p.score  = score;
    return p;
}

inline TimeSyncPacket MakeTimeSync(uint16_t remainSec)
{
    TimeSyncPacket p{};
    p.size = sizeof(TimeSyncPacket);
    p.type = SC_TIME_SYNC;
    p.remainSec = remainSec;
    return p;
}

inline GameEndPacket MakeGameEnd(GameResult result, EndReason reason, uint32_t myScore, uint32_t oppScore)
{
    GameEndPacket p{};
    p.size     = sizeof(GameEndPacket);
    p.type     = SC_GAME_END;
    p.result   = result;
    p.reason   = reason;
    p.myScore  = myScore;
    p.oppScore = oppScore;
    return p;
}

inline GameStartPacket MakeGameStart(uint32_t seed, uint16_t timeLimitSec, uint16_t countdownMs)
{
    GameStartPacket p{};
    p.size         = sizeof(GameStartPacket);
    p.type         = SC_GAME_START;
    p.seed         = seed;
    p.timeLimitSec = timeLimitSec;
    p.countdownMs  = countdownMs;
    return p;
}

inline PacketHeader MakeOppLeft()
{
    PacketHeader p{};
    p.size = sizeof(PacketHeader);
    p.type = SC_OPP_LEFT;
    return p;
}

// 바이트 순서 변환 ---------------------------------------------------------------------------------------
inline void ToNetwork(PacketHeader& h) { h.size = htons(h.size); h.type = htons(h.type); }
inline void ToHost(PacketHeader& h)    { h.size = ntohs(h.size); h.type = ntohs(h.type); }

inline void ToNetwork(GameEventPacket& p)
{
    ToNetwork(static_cast<PacketHeader&>(p));
    p.timeMs = htonl(p.timeMs);
    p.arg16  = htons(p.arg16);
    p.arg32  = (int32_t)htonl((uint32_t)p.arg32);
    p.score  = htonl(p.score);
}
inline void ToHost(GameEventPacket& p)
{
    ToHost(static_cast<PacketHeader&>(p));
    p.timeMs = ntohl(p.timeMs);
    p.arg16  = ntohs(p.arg16);
    p.arg32  = (int32_t)ntohl((uint32_t)p.arg32);
    p.score  = ntohl(p.score);
}

inline void ToNetwork(TimeSyncPacket& p) { ToNetwork(static_cast<PacketHeader&>(p)); p.remainSec = htons(p.remainSec); }
inline void ToHost(TimeSyncPacket& p)    { ToHost(static_cast<PacketHeader&>(p));    p.remainSec = ntohs(p.remainSec); }

inline void ToNetwork(GameEndPacket& p)
{
    ToNetwork(static_cast<PacketHeader&>(p));
    p.myScore  = htonl(p.myScore);
    p.oppScore = htonl(p.oppScore);
}
inline void ToHost(GameEndPacket& p)
{
    ToHost(static_cast<PacketHeader&>(p));
    p.myScore  = ntohl(p.myScore);
    p.oppScore = ntohl(p.oppScore);
}

inline void ToNetwork(GameStartPacket& p)
{
    ToNetwork(static_cast<PacketHeader&>(p));
    p.seed         = htonl(p.seed);
    p.timeLimitSec = htons(p.timeLimitSec);
    p.countdownMs  = htons(p.countdownMs);
}
inline void ToHost(GameStartPacket& p)
{
    ToHost(static_cast<PacketHeader&>(p));
    p.seed         = ntohl(p.seed);
    p.timeLimitSec = ntohs(p.timeLimitSec);
    p.countdownMs  = ntohs(p.countdownMs);
}

// 송신 -------------------------------------------------------------------------------------------------
inline bool SendPacket(SOCKET sock, const void* data, int len)
{
    const char* p = (const char*)data;
    while (len > 0) {
        int ret = send(sock, p, len, 0);
        if (ret == SOCKET_ERROR) return false;
        p   += ret;
        len -= ret;
    }
    return true;
}

// 패킷 송신용 템플릿
template <typename T>
inline bool SendTyped(SOCKET sock, T pkt)
{
    ToNetwork(pkt);
    return SendPacket(sock, &pkt, sizeof(T));
}

// 수신 ----------------------------------------------------------------------------------------------
inline int RecvPacket(SOCKET sock, char* buf, int bufSize)
{
    const int headerSize = (int)sizeof(PacketHeader);
    if (bufSize < headerSize) return -1;

    int ret = recv(sock, buf, headerSize, MSG_WAITALL);
    if (ret == 0) return 0;
    if (ret != headerSize) return -1;                      // 오류, 또는 헤더 도중에 끊김

    int size = ntohs(((PacketHeader*)buf)->size);
    if (size < headerSize || size > bufSize) return -1;    // 비정상 size → 호출한 쪽에서 연결 종료

    int bodySize = size - headerSize;
    if (bodySize > 0) {
        ret = recv(sock, buf + headerSize, bodySize, MSG_WAITALL);
        if (ret == 0) return 0;
        if (ret != bodySize) return -1;
    }
    return size;
}

inline uint16_t PeekType(const char* buf) { return ntohs(((const PacketHeader*)buf)->type); }
inline uint16_t PeekSize(const char* buf) { return ntohs(((const PacketHeader*)buf)->size); }

// 패킷 수신용 템플릿
// buf를 구조체로 꺼내서 호스트 순서로 변경. 크기가 다르면 false
template <typename T>
inline bool ReadTyped(const char* buf, int len, T& out)
{
    if (len != (int)sizeof(T)) return false;
    memcpy(&out, buf, sizeof(T));
    ToHost(out);
    return true;
}

inline bool IsValidEventType(uint8_t e) { return e < EVENT_COUNT; }
