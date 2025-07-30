#include "RetroGBm/Pch.h"
#include "RetroGBm/Tcp/TcpListener.h"
#include <iostream>

TcpListener::TcpListener(const std::string& ip, int port)
{
	this->m_HostIp = ip;
	this->m_HostPort = port;
}

TcpListener::~TcpListener()
{
	if (m_ServerSocket != INVALID_SOCKET)
	{
		closesocket(m_ServerSocket);
	}

	WSACleanup();
}

bool TcpListener::Start()
{
	// Create socket
	ADDRINFOW hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	PADDRINFOW result = NULL;
	int iResult = GetAddrInfoW(L"127.0.0.1", L"54000", &hints, &result);

	m_ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (m_ServerSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	// Bind the socket
	sockaddr_in serverAddr = {};
	int bind_result = bind(m_ServerSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
	if (bind_result == SOCKET_ERROR)
	{
		std::cerr << "Bind failed.\n";
		closesocket(m_ServerSocket);
		WSACleanup();
		return false;
	}

	// Listen
	listen(m_ServerSocket, SOMAXCONN);
	std::cout << "Server listening on 127.0.0.1:54000...\n";
	return true;
}

void TcpListener::Listen(std::function<void(SOCKET* client_socket, char data[512])> func)
{
	while (true)
	{
		// Accept a client connection
		sockaddr_in clientAddr = {};
		int addrSize = sizeof(clientAddr);
		SOCKET client_socket = accept(m_ServerSocket, (sockaddr*)&clientAddr, &addrSize);
		if (client_socket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed.\n";
			closesocket(m_ServerSocket);
			WSACleanup();
		}

		std::thread t([client_socket, func]() mutable
		{
			std::cout << "Client connected!\n";

			// Receive and echo data
			while (true)
			{
				char buffer[512] = {};
				int result = recv(client_socket, buffer, sizeof(buffer), 0);
				if (result == SOCKET_ERROR || result == 0)
				{
					std::cout << "Client disconnected or error occurred.\n";
					break;
				}

				func(&client_socket, buffer);
			}

			closesocket(client_socket);
		});

		t.detach();
	}
}