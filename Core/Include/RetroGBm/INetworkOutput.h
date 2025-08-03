#pragma once

#include <cstdint>

class INetworkOutput
{
public:
	INetworkOutput() = default;
	virtual ~INetworkOutput() = default;

	virtual void SendData(uint8_t data) = 0;
};