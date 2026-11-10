#include "../Protocol.h"
#include <ws2tcpip.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <atomic>

constexpr uint16_t SERVER_PORT = 9000;

const char* EVENT_NAME[EVENT_COUNT] = {
    "BLOCK_HIT", "BLOCK_BROKEN", "ITEM_GET", "ITEM_END",
    "BALL_LOST", "RESPAWN", "STAGE_CLEAR", "STAGE_START",
};
const char* RESULT_NAME[] = { "패배", "승리", "무승부" };
const char* REASON_NAME[] = { "시간 종료", "모두 클리어", "상대 이탈" };

std::atomic<bool>      g_connected{ true };
std::atomic<bool>      g_quitting{ false };    // q로 직접 종료하는 중
std::atomic<bool>      g_ended{ false };
std::atomic<long long> g_startTick{ 0 };   // 게임이 시작되는 시각 (GetTickCount64 기준). 0 = 아직 SC_GAME_START 안 받음

// 수신 스레드 -------------------------------------------------------------------------
DWORD WINAPI RecvThread(LPVOID arg)
{
    SOCKET sock = (SOCKET)arg;
    char buf[MAX_PACKET_SIZE];
    int n;

    while ((n = RecvPacket(sock, buf, sizeof(buf))) > 0) {
        switch (PeekType(buf)) {
        case SC_GAME_START: {
            GameStartPacket p;
            if (!ReadTyped(buf, n, p)) break;
            g_startTick = (long long)GetTickCount64() + p.countdownMs;
            printf("\n[서버] 게임 시작! seed=%08X 제한시간=%u초, %ums 후 시작\n",
                   p.seed, p.timeLimitSec, p.countdownMs);
            break;
        }
        case SC_OPP_EVENT: {
            GameEventPacket p;
            if (!ReadTyped(buf, n, p) || !IsValidEventType(p.event)) break;
            printf("\n[상대 이벤트] %s timeMs=%u arg8=%u arg16=%u arg32=%d score=%u\n",
                   EVENT_NAME[p.event], p.timeMs, p.arg8, p.arg16, p.arg32, p.score);
            break;
        }
        case SC_TIME_SYNC: {
            TimeSyncPacket p;
            if (ReadTyped(buf, n, p)) printf("\n[서버] 남은 시간 %u초\n", p.remainSec);
            break;
        }
        case SC_GAME_END: {
            GameEndPacket p;
            if (!ReadTyped(buf, n, p) || p.result > RESULT_DRAW || p.reason > REASON_OPP_LEFT) break;
            g_ended = true;
            printf("\n[서버] 게임 종료: %s (%s) 내 점수=%u 상대 점수=%u\n",
                   RESULT_NAME[p.result], REASON_NAME[p.reason], p.myScore, p.oppScore);
            break;
        }
        case SC_OPP_LEFT:
            printf("\n[서버] 상대 연결이 끊겼습니다\n");
            break;
        default:
            printf("\n[서버] 알 수 없는 type=%u (%d바이트)\n", PeekType(buf), n);
            break;
        }
    }

    if (!g_quitting)
        printf("\n서버 연결 끊김 (%s). 엔터를 누르면 종료합니다.\n", n == 0 ? "정상 종료" : "오류");
    g_connected = false;
    return 0;
}

void PrintMenu()
{
    printf("\n---------------- 이벤트 보내기 ----------------\n"
           " 1 BLOCK_HIT     2 BLOCK_BROKEN(+100)   3 ITEM_GET\n"
           " 4 ITEM_END      5 BALL_LOST            6 RESPAWN\n"
           " 7 STAGE_CLEAR(+500)                    8 STAGE_START\n"
           " 9 잘못된 event 값 (서버가 무시하는지 확인)\n"
           " m 메뉴 다시 보기   q 종료\n"
           "-----------------------------------------------\n");
}

int main(int argc, char* argv[])
{
    const char* serverIp = (argc > 1) ? argv[1] : "127.0.0.1";

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, serverIp, &addr.sin_addr) != 1) {
        printf("IP 주소 형식이 잘못됨: %s\n", serverIp);
        return 1;
    }
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("서버(%s:%u) 접속 실패: %d\n", serverIp, SERVER_PORT, WSAGetLastError());
        return 1;
    }
    printf("서버(%s:%u) 접속 완료. 상대가 들어오면 게임이 시작됩니다.\n", serverIp, SERVER_PORT);

    HANDLE recvThread = CreateThread(nullptr, 0, RecvThread, (LPVOID)sock, 0, nullptr);
    PrintMenu();

    uint32_t score = 0;     // 메인 스레드에서만 사용
    uint8_t  stage = 1;
    uint16_t blockIdx = 0;
    std::string line;

    while (g_connected && std::getline(std::cin, line)) {
        if (!g_connected) break;
        if (line.empty()) continue;
        char key = line[0];
        if (key == 'q') break;
        if (key == 'm') { PrintMenu(); continue; }

        long long start = g_startTick;
        long long now   = (long long)GetTickCount64();
        if (start == 0)  { printf("아직 게임 시작 전입니다 (상대 대기 중)\n"); continue; }
        if (now < start) { printf("시작까지 %lldms 남았습니다\n", start - now); continue; }
        if (g_ended)     { printf("게임이 끝났습니다\n"); continue; }

        uint32_t timeMs = (uint32_t)(now - start);
        GameEventPacket pkt;
        switch (key) {
        case '1': pkt = MakeGameEvent(BLOCK_HIT,    timeMs, score, 0, blockIdx, 1); break;
        case '2': score += 100;
                  pkt = MakeGameEvent(BLOCK_BROKEN, timeMs, score, 0, blockIdx++, 100); break;
        case '3': pkt = MakeGameEvent(ITEM_GET,     timeMs, score, 1); break;
        case '4': pkt = MakeGameEvent(ITEM_END,     timeMs, score, 1); break;
        case '5': pkt = MakeGameEvent(BALL_LOST,    timeMs, score, 0, 0, 3000); break;
        case '6': pkt = MakeGameEvent(RESPAWN,      timeMs, score); break;
        case '7': score += 500;
                  pkt = MakeGameEvent(STAGE_CLEAR,  timeMs, score, stage, 2, 500); break;
        case '8': stage++;
                  pkt = MakeGameEvent(STAGE_START,  timeMs, score, stage); break;
        case '9': pkt = MakeGameEvent((EventType)200, timeMs, score); break;
        default:  printf("1~9, m, q 중에서 입력하세요\n"); continue;
        }

        if (!SendTyped(sock, pkt)) { printf("전송 실패: %d\n", WSAGetLastError()); break; }
        printf("[보냄] %s timeMs=%u score=%u\n",
               IsValidEventType(pkt.event) ? EVENT_NAME[pkt.event] : "(잘못된 값)", timeMs, score);
    }

    g_quitting = true;
    shutdown(sock, SD_BOTH);           // 수신 스레드의 recv를 깨워서 끝나게 함
    WaitForSingleObject(recvThread, 3000);
    CloseHandle(recvThread);
    closesocket(sock);
    WSACleanup();
    return 0;
}
