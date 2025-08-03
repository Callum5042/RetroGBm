#pragma once

#include <RetroGBm/INetworkOutput.h>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

class WinNetworkOutput : public INetworkOutput
{
public:
	WinNetworkOutput() = default;
	virtual ~WinNetworkOutput();

	void SendData(uint8_t data) override;
	uint8_t ReceiveData() override;

	void CreateHost(const std::string& ip);
	void CreateClient(const std::string& ip);

private:
	SOCKET m_PeerSocket = INVALID_SOCKET;

	int m_DefaultPort = 54000;
};