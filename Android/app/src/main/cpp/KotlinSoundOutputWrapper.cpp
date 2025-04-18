#include "KotlinSoundOutputWrapper.h"
#include "JniLoader.h"

KotlinSoundOutputWrapper::KotlinSoundOutputWrapper(JNIEnv* env, jobject javaSoundOutput) {
    this->javaSoundOutput = env->NewGlobalRef(javaSoundOutput);

    jclass cls = env->GetObjectClass(javaSoundOutput);
    methodStart = env->GetMethodID(cls, "start", "()V");
    methodStop = env->GetMethodID(cls, "stop", "()V");
    methodPlayBuffered = env->GetMethodID(cls, "playBufferedSamples", "([B)V");

    m_AudioBuffer.resize(BufferSize, 128);

    const int CPU_FREQUENCY = 4 * 1024 * 1024;
    _divider = CPU_FREQUENCY / SampleRate;
}

KotlinSoundOutputWrapper::~KotlinSoundOutputWrapper() {
    JNIEnv* env = GetJNIEnv();
    env->DeleteGlobalRef(javaSoundOutput);
}

void KotlinSoundOutputWrapper::Start() {
    JNIEnv* env = GetJNIEnv();
    env->CallVoidMethod(javaSoundOutput, methodStart);
}

void KotlinSoundOutputWrapper::Stop() {
    JNIEnv* env = GetJNIEnv();
    env->CallVoidMethod(javaSoundOutput, methodStop);
}

void KotlinSoundOutputWrapper::Play(int left, int right) {

    // Only want to add sample to the buffer synced with CPU
    if (_tick++ != 0)
    {
        _tick %= _divider;
        return;
    }

    // Populate buffer
    m_AudioBuffer[m_SampleCount++] = static_cast<jbyte>(left - 128);
    m_AudioBuffer[m_SampleCount++] = static_cast<jbyte>(right - 128);

    // Queue audio buffer
    if (m_SampleCount >= BufferSize)
    {
        JNIEnv* env = GetJNIEnv();

        jbyteArray jbuffer = env->NewByteArray(m_AudioBuffer.size());

        env->SetByteArrayRegion(jbuffer, 0, m_AudioBuffer.size(), m_AudioBuffer.data());
        env->CallVoidMethod(javaSoundOutput, methodPlayBuffered, jbuffer);
        env->DeleteLocalRef(jbuffer);

        m_SampleCount = 0;
    }
}