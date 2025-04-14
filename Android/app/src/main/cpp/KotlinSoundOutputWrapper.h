#pragma once

#include <jni.h>
#include <vector>

#include <RetroGBm/Audio/ISoundOutput.h>

class KotlinSoundOutputWrapper : public ISoundOutput
{
public:
    KotlinSoundOutputWrapper(JNIEnv* env, jobject javaSoundOutput);
    ~KotlinSoundOutputWrapper() override;

    void Start() override;
    void Stop() override;
    void Play(int left, int right) override;

private:
    jobject javaSoundOutput;
    jmethodID methodStart;
    jmethodID methodStop;
    jmethodID methodPlayBuffered;

    jbyteArray jbuffer;

    const int BufferSize = 1024;
    const int SampleRate = 22050;

// Things
    std::vector<jbyte> m_AudioBuffer;
    int m_SampleCount = 0;
    int _tick = 0;
    int _divider = 0;


};