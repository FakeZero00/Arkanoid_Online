#include "AllNetworkLib.h"
#include <iostream>
#include <map>
#include <vector>

#define SERVERPORT 9090

std::map<char*, Session*> ip2session;
std::map<Session*, char*> session2ip;
std::map<Session*, Room*> session2rooms;
std::vector<Room* > rooms;

DWORD WINAPI SessionThread(LPVOID arg);

int main() {
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		err_quit("WSAStartup()");
		return 1;
	}

	//소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
		return 1;
	}

	//bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	//데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	while (1) {
		//accept()
		std::cout << "[TCP 서버] 클라이언트 접속 대기 중..." << std::endl;
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		std::cout << "[TCP 서버] 클라이언트 접속: IP 주소=" << addr << ", 포트=" << ntohs(clientaddr.sin_port) << std::endl;

		// 세션 생성 및 스레드 시작
		Session* session = new Session();
		session->SetSocket(client_sock);
		ip2session[addr] = session;
		session2ip[session] = addr;
		HANDLE h = CreateThread(NULL, 0, SessionThread, session, 0, NULL);
		if (h) CloseHandle(h);
	}
}

DWORD WINAPI SessionThread(LPVOID arg)
{
	Session* s = (Session*)arg;
	char buf[MAX_PACKET_SIZE];
	int n;

	while ((n = RecvPacket(s->GetSocket(), buf, sizeof(buf))) > 0) {
		uint16_t type = PeekType(buf);

		if (type == CS_LOBBY_INFO) {
			RoomInfoPacket pkt;
			if (ReadTyped(buf, n, pkt)) {
				printf("[세션 %d] 방 이름=%s, 플레이어 이름=%s\n", s->GetId(), pkt.roomName, pkt.playerName);

				// 방 생성 및 세션-방 매핑
				Room* room = new Room(s);
				strncpy(room->GetRoomName(), pkt.roomName, sizeof(room->GetRoomName()) - 1);
				room->GetRoomName()[sizeof(room->GetRoomName()) - 1] = '\0';
				session2rooms[s] = room;
				rooms.push_back(room);
			}
			else printf("[세션 %d] RoomInfoPacket 크기가 다름 (%d바이트) → 무시\n", s->GetId(), n);
		}

		else if (type == CS_REFRESH_REQUEST) {
			printf("[세션 %d] 방 목록 새로고침 요청\n", s->GetId());
			SendRoomList(s->GetSocket(), rooms);
		}
		
		/*Room* room = GetRoom(s);
		if (!room) {
			printf("[세션 %d] 방이 없는데 인게임 패킷 type=%u → 무시\n", s->GetId(), type);
			continue;
		}*/

		/*if (type == CS_GAME_EVENT) {
			GameEventPacket pkt;
			if (ReadTyped(buf, n, pkt)) room->OnGameEvent(s, pkt);
			else printf("[세션 %d] CS_GAME_EVENT 크기가 다름 (%d바이트) → 무시\n", s->GetId(), n);
		}*/
		
		else {
			printf("[세션 %d] 클라이언트가 보낼 수 없는 type=%u → 무시\n", s->GetId(), type);
		}
	}

	printf("[세션 %d] 연결 끊김 (%s)\n", s->GetId(), n == 0 ? "정상 종료" : "오류 또는 비정상 패킷");

	s->Close();
	// 상대 스레드가 아직 이 Session을 가리킬 수 있어서 테스트 서버에서는 delete 하지 않음
	return 0;
}