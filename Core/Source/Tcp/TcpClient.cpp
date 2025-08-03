#include "RetroGBm/Pch.h"
#include "RetroGBm/Tcp/TcpClient.h"
#include <iostream>

TcpClient::TcpClient(const std::string& ip, int port)
{
    this->m_HostIp = ip;
    this->m_HostPort = port;
}

TcpClient::~TcpClient()
{
    if (m_Socket != INVALID_SOCKET)
    {
        closesocket(m_Socket);
    }

    WSACleanup();
}

bool TcpClient::Start()
{
    // Initialize Winsock
    WSADATA wsa_data = {};
    int startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (startup_result != 0)
    {
        std::cerr << "WSAStartup failed: " << startup_result << "\n";
        return false;
    }

    // Create a socket
    ADDRINFOW hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    PADDRINFOW result = NULL;
    int iResult = GetAddrInfoW(L"127.0.0.1", L"54000", &hints, &result);

    m_Socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_Socket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // Connect to the server
    int socket_result = connect(m_Socket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (socket_result == SOCKET_ERROR)
    {
        std::cerr << "Unable to connect to server: " << WSAGetLastError() << "\n";
        closesocket(m_Socket);
        WSACleanup();
        return false;
    }

    return true;
}

void TcpClient::Send(const uint8_t data) const
{
	uint8_t data_array[2] = { 0xFF, data };
    send(m_Socket, reinterpret_cast<const char*>(data_array), sizeof(data_array), 0);
}

uint8_t TcpClient::ReceiveByte() const
{
    char buffer[1024] = { 0 };
    int bytes_received = recv(m_Socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        return buffer[1];
    }
    else if (bytes_received == 0)
    {
        std::cerr << "Connection closed by server.\n";
        return 0;
    }
    else
    {
        std::cerr << "recv failed: " << WSAGetLastError() << "\n";
        return 0;
    }
}
