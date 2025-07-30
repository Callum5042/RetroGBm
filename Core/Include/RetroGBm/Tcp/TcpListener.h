#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <functional>
#include <vector>
#include <thread>

class TcpListener
{
public:
	TcpListener(const std::string& ip, int port);
	virtual ~TcpListener();

	bool Start();
	void Listen(std::function<void(SOCKET* client_socket, char data[512])> func);

private:
	std::string m_HostIp;
	int m_HostPort = 0;

	SOCKET m_ServerSocket = INVALID_SOCKET;
	std::vector<std::thread> m_Threads;
};