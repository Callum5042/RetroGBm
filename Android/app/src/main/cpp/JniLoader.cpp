#include "JniLoader.h"

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_javaVM = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEnv* GetJNIEnv() {
    JNIEnv* env = nullptr;
    if (g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        g_javaVM->AttachCurrentThread(&env, nullptr);
    }

    return env;
}