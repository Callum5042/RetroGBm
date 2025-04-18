#pragma once

#include <jni.h>
#include <RetroGBm/IDisplayOutput.h>

class KotlinDisplayOutputWrapper : public IDisplayOutput
{
public:
    KotlinDisplayOutputWrapper(JNIEnv* env, jobject javaDisplayOutput);
    ~KotlinDisplayOutputWrapper() override;

    void Draw(void* pixel_buffer, int pixel_pitch) override;

private:
    jobject javaDisplayOutput;
    jmethodID methodDraw;
};