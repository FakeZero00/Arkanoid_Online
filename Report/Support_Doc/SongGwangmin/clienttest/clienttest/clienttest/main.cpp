#include <iostream>

#include <chrono>
#include <thread>

#include "Vec2D.h" // Vec2D 클래스 정의 포함
#include "Common.h"

#include <Windows.h> // Windows API 헤더 - Common.h보다 늦게 포함해야 함 (Common.h에서 winsock2.h 포함됨)

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

int main() {
    
    std::cout << "=========================================" << std::endl;
    std::cout << " deltaTime 기반 2D Vector 이동 테스트" << std::endl;
    std::cout << " 방향키(UP, DOWN, LEFT, RIGHT)로 이동" << std::endl;
    std::cout << " 종료하려면 ESC 키를 누르세요." << std::endl;
    std::cout << "=========================================\n" << std::endl;
    
    int retval;
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 소켓 주소 구조체 초기화
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    // 데이터 통신에 사용할 변수
    struct sockaddr_in peeraddr;
    int addrlen;
    char buf[BUFSIZE + 1];
    int len;
    

    // 초기 상태 정의
    Vec2D position(0.0f, 0.0f); // 초기 위치 (0, 0)
    float speed = 10.0f;           // 초당 이동 속도 (1초에 10단위 이동)

    // 시간 계산을 위한 변수 초기화 (Chrono 라이브러리 사용)
    auto previousTime = std::chrono::high_resolution_clock::now();

	Vec2DPacket packet; // 서버로 전송할 위치 데이터 패킷
    
    while (true) {
        // --- 1. Delta Time 계산 ---
        auto currentTime = std::chrono::high_resolution_clock::now();
        // 두 프레임 사이의 시간 차이를 초(second) 단위로 계산
        std::chrono::duration<float> elapsedTime = currentTime - previousTime;
        float deltaTime = elapsedTime.count();
        previousTime = currentTime;

        // --- 2. 입력 처리 (방향 벡터 설정) ---
        Vec2D direction(0.0f, 0.0f);

        if (GetAsyncKeyState(VK_UP) & 0x8000)    direction.y += 1.0f;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)  direction.y -= 1.0f;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  direction.x -= 1.0f;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) direction.x += 1.0f;

        // ESC 키 탈출
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            printf("\n프로그램을 종료합니다.\n");
            break;
        }

        // --- 3. 위치 업데이트 (공식: 현재위치 += 방향 * 속도 * deltaTime) ---
        if (direction.x != 0.0f || direction.y != 0.0f) {
            // 위치 업데이트 적용
            position += direction * speed * deltaTime;

            // 콘솔창에 실시간 좌표 출력 (\r을 사용해 한 줄에서 갱신되도록 처리)
            printf("\r[위치] X: %f,\tY: %f\t(dt: %f)", position.x, position.y, deltaTime);

			// 소켓을 통해 서버로 위치 데이터 전송
            
            packet.x = position.x;
            packet.y = position.y;

            retval = sendto(sock, (char*)(&packet), sizeof(packet), 0,
                (struct sockaddr*)&serveraddr, sizeof(serveraddr));
            if (retval == SOCKET_ERROR) {
                err_display("sendto()");
                break;
            }
        }

        // --- 4. 프레임 제한 (CPU 점유율 방지용, 성능이 달라도 dt 덕분에 이동 거리는 일정함) ---
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 대략 60 FPS 타겟
    }
    
    // 소켓 닫기
    closesocket(sock);

    // 윈속 종료
    WSACleanup();

    return 0;
}