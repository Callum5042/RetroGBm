#pragma once

#include <jni.h>
#include <RetroGBm/INetworkOutput.h>

class KotlinNetworkOutputWrapper : public INetworkOutput
{
public:
    KotlinNetworkOutputWrapper(JNIEnv* env, jobject javaNetworkOutput);
    ~KotlinNetworkOutputWrapper() override = default;

    void SendData(uint8_t data) override;

private:
    jobject javaNetworkOutput;
    jmethodID methodSenddata;
};