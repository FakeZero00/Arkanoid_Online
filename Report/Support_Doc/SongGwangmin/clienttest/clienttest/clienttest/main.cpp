#include <iostream>
#include <Windows.h> // Windows API 헤더
#include <chrono>
#include <thread>

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << " Windows API 기반 실시간 비동기 키 입력 테스트" << std::endl;
    std::cout << " 종료하려면 ESC 키를 누르세요." << std::endl;
    std::cout << "=========================================\n" << std::endl;

    while (true) {
        // GetAsyncKeyState의 최상위 비트(0x8000)가 1이면 현재 키가 눌려 있는 상태입니다.
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            std::cout << "[유지] 방향키 위 (UP)" << std::endl;
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            std::cout << "[유지] 방향키 아래 (DOWN)" << std::endl;
        }
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
            std::cout << "[유지] 방향키 왼쪽 (LEFT)" << std::endl;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            std::cout << "[유지] 방향키 오른쪽 (RIGHT)" << std::endl;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            std::cout << "[유지] 스페이스바 (SPACE)" << std::endl;
        }
        if (GetAsyncKeyState('W') & 0x8000) {
            std::cout << "[유지] W 키" << std::endl;
        }

        // ESC 키 탈출
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "ESC가 눌려 프로그램을 종료합니다." << std::endl;
            break;
        }

        // 무한 루프로 인한 CPU 과점유 방지 (약 30ms 대기)
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return 0;
}