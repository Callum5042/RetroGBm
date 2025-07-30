#include "RetroGBm/Pch.h"
#include "TcpClient.h"
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

void TcpClient::Send(const std::string& message) const
{
    send(m_Socket, message.data(), message.size(), 0);
}

std::string TcpClient::Receive() const
{
    char buffer[1024] = { 0 };
    int bytes_received = recv(m_Socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0'; // Null-terminate the string
        return std::string(buffer);
    }
    else if (bytes_received == 0)
    {
        std::cerr << "Connection closed by server.\n";
        return "";
    }
    else
    {
        std::cerr << "recv failed: " << WSAGetLastError() << "\n";
        return "";
    }
}