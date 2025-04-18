#pragma once

#include <jni.h>

static JavaVM* g_javaVM = nullptr;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved);

extern "C" JNIEnv* GetJNIEnv();