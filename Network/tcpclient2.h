//
// 2016-07-05, jjuiddong
// 
// cTCPClient 를 대체하기위해 만들어졌다.
// Connection 을 쓰레드로 처리한다.
//
#pragma once

#include "packetqueue.h"

namespace network
{

	class cTCPClient2
	{
	public:
		cTCPClient2();
		virtual ~cTCPClient2();

		bool Init(const string &ip, const int port
			, const int packetSize = 512, const int maxPacketCount = 10, const int sleepMillis = 30);
		void Send(const char protocol[4], const BYTE *buff, const int len);
		void Close();
		bool IsConnect() const;
		bool IsReadyConnect() const;
		bool IsFailConnection() const;


	public:
		enum STATE {
			READYCONNECT,  // 커넥션 전, 쓰레드 구동하기 직전.
			TRYCONNECT,  // 쓰레드 내에서 소켓 커넥션 요청 상태
			CONNECT,  // 커넥션 성공
			DISCONNECT, // 커넥션 실패
			CONNECT_ERROR, // 커넥션 에러, 주로 동기화 문제에서 발생
		};
		 
		STATE m_state;
		string m_ip;
		int m_port;
		SOCKET m_socket; // server socket
		int m_maxBuffLen;
		cPacketQueue m_sendQueue;
		cPacketQueue m_recvQueue;

		std::thread m_thread;
		int m_sleepMillis;
		int m_recvBytes; // debug
	};


	inline bool cTCPClient2::IsConnect() const { return m_state == CONNECT; }
	inline bool cTCPClient2::IsReadyConnect() const { return (m_state == READYCONNECT) || (m_state == TRYCONNECT); }
	inline bool cTCPClient2::IsFailConnection() const {return m_state == DISCONNECT; }
}
