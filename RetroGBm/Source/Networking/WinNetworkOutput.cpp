#include "Networking/WinNetworkOutput.h"
#include <RetroGBm/Logger.h>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cpu.h>

namespace
{
	// TODO: This should live somewhere else
	DWORD WINAPI ReceiveMessages(LPVOID lpParam)
	{
		SOCKET sock = *(SOCKET*)lpParam;
		char buffer[1024];

		while (true)
		{
			int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
			if (bytesReceived <= 0)
			{
				Logger::Info("Disconnected");
				break;
			}

			Emulator::Instance->m_SerialData[0] = buffer[1];

			Emulator::Instance->m_SerialData[1] &= ~0x80;
			Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::Serial);
		}

		closesocket(sock);
		return 0;
	}
}

WinNetworkOutput::~WinNetworkOutput()
{
	if (m_PeerSocket != INVALID_SOCKET)
	{
		closesocket(m_PeerSocket);
		m_PeerSocket = INVALID_SOCKET;
	}
}

void WinNetworkOutput::SendData(uint8_t data)
{
	uint8_t data_array[2] = { 0xFF, data };
	send(m_PeerSocket, reinterpret_cast<const char*>(data_array), sizeof(data_array), 0);
}

uint8_t WinNetworkOutput::ReceiveData()
{
	return 0;
}

void WinNetworkOutput::CreateHost(const std::string& ip)
{
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_DefaultPort);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(listenSocket, (sockaddr*)&addr, sizeof(addr));
	listen(listenSocket, 1);

	Logger::Info("Listening on port " + std::to_string(m_DefaultPort) + "...");

	m_PeerSocket = accept(listenSocket, NULL, NULL);
	Logger::Info("Peer connected.");

	CreateThread(NULL, 0, ReceiveMessages, &m_PeerSocket, 0, NULL);
}

void WinNetworkOutput::CreateClient(const std::string& ip)
{
	m_PeerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_DefaultPort);
	serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	Logger::Info("Connecting to " + ip + ":" + std::to_string(m_DefaultPort));

	if (connect(m_PeerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		Logger::Error("Failed to connect to " + ip + ":" + std::to_string(m_DefaultPort));
		exit(1);
	}

	Logger::Info("Connected to " + ip + ":" + std::to_string(m_DefaultPort));

	CreateThread(NULL, 0, ReceiveMessages, &m_PeerSocket, 0, NULL);
}