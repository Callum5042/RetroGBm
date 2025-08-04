#pragma once

#include <RetroGBm/INetworkOutput.h>

class KotlinNetworkOutputWrapper : public INetworkOutput
{
public:
    KotlinNetworkOutputWrapper() = default;
    ~KotlinNetworkOutputWrapper() override = default;

    void SendData(uint8_t data) override
    {
        // Do nothing
    }
};