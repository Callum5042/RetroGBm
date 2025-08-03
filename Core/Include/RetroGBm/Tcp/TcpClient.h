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

	void Send(const uint8_t data) const;
	uint8_t ReceiveByte() const;

private:
	std::string m_HostIp;
	int m_HostPort = 0;

	SOCKET m_Socket = 0;
};