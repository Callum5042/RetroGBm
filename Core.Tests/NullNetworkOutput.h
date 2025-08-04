#pragma once

#include <RetroGBm/INetworkOutput.h>

class NullNetworkOutput : public INetworkOutput
{
public:
	NullNetworkOutput() = default;
	virtual ~NullNetworkOutput() = default;

	void SendData(uint8_t data) override
	{
		// No operation for null output
	}
};