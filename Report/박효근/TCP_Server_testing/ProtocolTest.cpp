#include "Protocol.h"
#include <cstdio>
#include <cstring>

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(ls, (sockaddr*)&addr, sizeof(addr));
    listen(ls, 1);
    int alen = sizeof(addr);
    getsockname(ls, (sockaddr*)&addr, &alen);

    SOCKET c = socket(AF_INET, SOCK_STREAM, 0);
    connect(c, (sockaddr*)&addr, sizeof(addr));
    SOCKET s = accept(ls, nullptr, nullptr);

    int fail = 0;
    char buf[MAX_PACKET_SIZE];

    GameEventPacket ev = MakeGameEvent(STAGE_CLEAR, 123456, 98765, 3, 2, -7);
    SendTyped(c, ev);
    int n = RecvPacket(s, buf, sizeof(buf));
    GameEventPacket got{};
    if (n != 20 || PeekType(buf) != CS_GAME_EVENT || !ReadTyped(buf, n, got) || memcmp(&got, &ev, sizeof(ev)) != 0) fail++;
    printf("event : n=%d type=%u timeMs=%u arg8=%u arg16=%u arg32=%d score=%u\n",
           n, got.type, got.timeMs, got.arg8, got.arg16, got.arg32, got.score);

    SendTyped(s, MakeTimeSync(175));
    n = RecvPacket(c, buf, sizeof(buf));
    TimeSyncPacket ts{};
    if (n != 6 || !ReadTyped(buf, n, ts) || ts.remainSec != 175) fail++;
    printf("sync  : n=%d remainSec=%u\n", n, ts.remainSec);

    SendTyped(s, MakeGameEnd(RESULT_WIN, REASON_TIME_OVER, 5000, 4200));
    n = RecvPacket(c, buf, sizeof(buf));
    GameEndPacket ge{};
    if (n != 14 || !ReadTyped(buf, n, ge) || ge.result != RESULT_WIN || ge.myScore != 5000 || ge.oppScore != 4200) fail++;
    printf("end   : n=%d result=%u my=%u opp=%u\n", n, ge.result, ge.myScore, ge.oppScore);

    SendTyped(s, MakeOppLeft());
    n = RecvPacket(c, buf, sizeof(buf));
    if (n != 4 || PeekType(buf) != SC_OPP_LEFT) fail++;
    printf("left  : n=%d type=%u\n", n, PeekType(buf));

    PacketHeader bad{ 3, CS_GAME_EVENT };              // size가 헤더보다 작음
    SendTyped(c, bad);
    n = RecvPacket(s, buf, sizeof(buf));
    if (n != -1) fail++;
    printf("bad   : n=%d\n", n);

    closesocket(c);
    n = RecvPacket(s, buf, sizeof(buf));
    if (n != 0) fail++;
    printf("close : n=%d\n", n);

    closesocket(s);
    closesocket(ls);
    WSACleanup();
    printf(fail ? "FAIL %d\n" : "ALL OK\n", fail);
    return fail;
}
