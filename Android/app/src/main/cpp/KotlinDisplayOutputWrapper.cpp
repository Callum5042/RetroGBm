#include "KotlinDisplayOutputWrapper.h"
#include "JniLoader.h"

KotlinDisplayOutputWrapper::KotlinDisplayOutputWrapper(JNIEnv* env, jobject javaDisplayOutput) {
    this->javaDisplayOutput = env->NewGlobalRef(javaDisplayOutput);
    jclass cls = env->GetObjectClass(javaDisplayOutput);

    methodDraw = env->GetMethodID(cls, "draw", "([I)V");
}

KotlinDisplayOutputWrapper::~KotlinDisplayOutputWrapper() {
    JNIEnv* env = GetJNIEnv();
    env->DeleteGlobalRef(javaDisplayOutput);
}

void KotlinDisplayOutputWrapper::Draw(void* pixel_buffer, int pixel_pitch) {
    JNIEnv* env = GetJNIEnv();

    jintArray jbuffer = env->NewIntArray(160 * 144);

    env->SetIntArrayRegion(jbuffer, 0, 160 * 144, reinterpret_cast<jint*>(pixel_buffer));
    env->CallVoidMethod(javaDisplayOutput, methodDraw, jbuffer);
    env->DeleteLocalRef(jbuffer);
}