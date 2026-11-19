#pragma once
#include "Protocol.h"
#include "Network.h"
#include <string>

#pragma pack(push, 1)
struct RoomInfoPacket : PacketHeader {
	char serverName[32];			// 서버 이름
	char playerName[32];			// 플레이어 이름
};
#pragma pack(pop)

static_assert(sizeof(RoomInfoPacket) == 68, "RoomInfoPacket 크기 확인");

bool SendRoomInfo(SOCKET sock, const char* serverName, const char* playerName)
{
	RoomInfoPacket p{};
	p.size = sizeof(RoomInfoPacket);
	p.type = 2026;
	// 문자열 복사
	strncpy_s(p.serverName, serverName, sizeof(p.serverName) - 1);
	strncpy_s(p.playerName, playerName, sizeof(p.playerName) - 1);
	p.serverName[sizeof(p.serverName) - 1] = '\0';
	p.playerName[sizeof(p.playerName) - 1] = '\0';

	return SendTyped(sock, p);
}