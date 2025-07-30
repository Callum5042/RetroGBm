#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

class TcpClient
{
public:
	TcpClient(const std::string& ip, int port);
	virtual ~TcpClient();

	bool Start();

	void Send(const std::string& message) const;

	std::string Receive() const;


private:
	std::string m_HostIp;
	int m_HostPort = 0;

	SOCKET m_Socket = 0;
};