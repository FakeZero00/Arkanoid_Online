#pragma once
#include <cstdint>

// 서버는 점수를 계산하지 않고, 클라이언트가 보낸 값을 검사한 뒤 받아 적기만 함
struct PlayerGameState {
    uint32_t score = 0;             // 마지막으로 받은 총점
    uint8_t  stage = 1;             // 지금 플레이 중인 스테이지
    bool     cleared = false;       // 지금 스테이지를 클리어했는지 (STAGE_START를 받으면 다시 false)
    bool     finished = false;      // 마지막 스테이지까지 클리어
    uint32_t lastTimeMs = 0;        // 마지막으로 받은 이벤트의 timeMs
};