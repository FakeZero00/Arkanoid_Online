#include <iostream>
#include <Windows.h> // Windows API 헤더
#include <chrono>
#include <thread>

#include "Vec2D.h" // Vec2D 클래스 정의 포함

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << " deltaTime 기반 2D Vector 이동 테스트" << std::endl;
    std::cout << " 방향키(UP, DOWN, LEFT, RIGHT)로 이동" << std::endl;
    std::cout << " 종료하려면 ESC 키를 누르세요." << std::endl;
    std::cout << "=========================================\n" << std::endl;

    // 초기 상태 정의
    Vec2D position(0.0f, 0.0f); // 초기 위치 (0, 0)
    float speed = 10.0f;           // 초당 이동 속도 (1초에 10단위 이동)

    // 시간 계산을 위한 변수 초기화 (Chrono 라이브러리 사용)
    auto previousTime = std::chrono::high_resolution_clock::now();

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
            std::cout << "\n프로그램을 종료합니다." << std::endl;
            break;
        }

        // --- 3. 위치 업데이트 (공식: 현재위치 += 방향 * 속도 * deltaTime) ---
        if (direction.x != 0.0f || direction.y != 0.0f) {
            // 위치 업데이트 적용
            position += direction * speed * deltaTime;

            // 콘솔창에 실시간 좌표 출력 (\r을 사용해 한 줄에서 갱신되도록 처리)
            std::cout << "\r[위치] X: " << position.x << ",\tY: " << position.y
                << "\t(dt: " << deltaTime << "s)" << std::flush;
        }

        // --- 4. 프레임 제한 (CPU 점유율 방지용, 성능이 달라도 dt 덕분에 이동 거리는 일정함) ---
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 대략 60 FPS 타겟
    }

    return 0;
}