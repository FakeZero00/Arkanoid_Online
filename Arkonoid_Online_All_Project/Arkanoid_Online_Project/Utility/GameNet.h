#pragma once
// ----------------------------------------------------------------------------------
//  GameNet  -  클라이언트 인게임 TCP 송신 (박효근)
//
//  - 게임 로직(스크립트)은 이 함수들만 부르면 됨. 패킷 · 소켓 · timeMs는 여기서 처리
//  - SC_GAME_START를 받기 전(혼자 테스트 중)에는 보내지 않고 false만 돌려줌
//  - 이 헤더는 winsock을 include 하지 않음 (게임 헤더와 충돌 방지). 패킷은 GameNet.cpp에서
// ----------------------------------------------------------------------------------
#include <cstdint>

// 블록 점수 (임시값. 팀에서 점수 규칙이 정해지면 수정)
constexpr int BLOCK_SCORE_NORMAL = 100;
constexpr int BLOCK_SCORE_SILVER = 200;     // 2번 맞아야 깨지는 블록

namespace GameNet {

// 게임 시작 · 종료 (11/22 수신 스레드에서 호출) -----------------------------------
void     OnGameStart(uint16_t timeLimitSec, uint16_t countdownMs);  // SC_GAME_START 받았을 때
void     OnGameEnd();                                               // SC_GAME_END 받았을 때
bool     IsPlaying();                   // 시작 후 ~ 종료 전
uint32_t GetTimeMs();                   // 게임 시작 후 경과 시간 (대기 시간 중에는 0)
uint32_t GetScore();                    // 내 총점

// 이벤트 송신 (메인 스레드 = 게임 로직에서 호출) ------------------------------------
// 성공하면 true. 게임 중이 아니거나 전송 실패면 false (게임은 계속 진행)
bool SendBlockHit(uint16_t blockIdx, int remainHp);        // 맞았지만 안 깨짐
bool SendBlockBroken(uint16_t blockIdx, int points);       // 깨짐. 점수를 더한 뒤 보냄
bool SendItemGet(uint8_t itemType);
bool SendItemEnd(uint8_t itemType);
bool SendBallLost(int respawnMs);
bool SendRespawn();
bool SendStageClear(uint8_t stage, uint16_t ballsLeft, int bonus);  // 보너스를 더한 뒤 보냄
bool SendStageStart(uint8_t stage);

} // namespace GameNet
