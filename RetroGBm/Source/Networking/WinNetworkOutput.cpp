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

	if (m_ListenSocket != NULL)
	{
		closesocket(m_ListenSocket);
		m_ListenSocket = NULL;
	}

	if (m_ReceiveThread.joinable())
	{
		m_ReceiveThread.join();
	}
}

void WinNetworkOutput::SendData(uint8_t data)
{
	uint8_t data_array[2] = { 0xFF, data };
	send(m_PeerSocket, reinterpret_cast<const char*>(data_array), sizeof(data_array), 0);
}

void WinNetworkOutput::CreateHost(const std::string& ip)
{
	// Create socket
	m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ListenSocket == INVALID_SOCKET)
	{
		Logger::Error("Failed to create socket.");
		return;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_DefaultPort);
	addr.sin_addr.s_addr = INADDR_ANY;

	int bind_result = bind(m_ListenSocket, (sockaddr*)&addr, sizeof(addr));
	if (bind_result == SOCKET_ERROR)
	{
		Logger::Error("Failed to bind socket to port " + std::to_string(m_DefaultPort));
		return;
	}

	// Start listening for connections
	listen(m_ListenSocket, SOMAXCONN);
	Logger::Info("Listening on port " + std::to_string(m_DefaultPort) + "...");

	// Wait for peer to connect
	m_ReceiveThread = std::thread([&]
	{
		m_PeerSocket = accept(m_ListenSocket, NULL, NULL);
		Logger::Info("Peer connected.");

		closesocket(m_ListenSocket);
		m_ListenSocket = NULL;

		CreateThread(NULL, 0, ReceiveMessages, &m_PeerSocket, 0, NULL);
	});
}

void WinNetworkOutput::CreateClient(const std::string& ip)
{
	// Create socket
	m_PeerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_PeerSocket == INVALID_SOCKET)
	{
		Logger::Error("Failed to create socket.");
		return;
	}

	// Connect to the host
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_DefaultPort);
	serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	Logger::Info("Connecting to " + ip + ":" + std::to_string(m_DefaultPort));
	if (connect(m_PeerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		Logger::Error("Failed to connect to " + ip + ":" + std::to_string(m_DefaultPort));
		closesocket(m_PeerSocket);
		return;
	}

	Logger::Info("Connected to " + ip + ":" + std::to_string(m_DefaultPort));
	CreateThread(NULL, 0, ReceiveMessages, &m_PeerSocket, 0, NULL);
}