#include "KotlinSoundOutputWrapper.h"

namespace {

    JavaVM* g_javaVM = nullptr;

    JNIEnv* GetJNIEnv() {
        JNIEnv* env = nullptr;
        if (g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            // Attach current thread if not already attached
            g_javaVM->AttachCurrentThread(&env, nullptr);
        }
        return env;
    }

    jshort convertToPcm16(uint8_t value) {
        return static_cast<jshort>((value - 128) * 256);
    }
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_javaVM = vm;
    return JNI_VERSION_1_6;
}

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
    JNIEnv* env = GetJNIEnv(); // You'll need to define this

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