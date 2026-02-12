#include "KotlinNetworkOutputWrapper.h"
#include "JniLoader.h"

KotlinNetworkOutputWrapper::KotlinNetworkOutputWrapper(JNIEnv *env, jobject javaNetworkOutput) {
    this->javaNetworkOutput = env->NewGlobalRef(javaNetworkOutput);

    jclass cls = env->GetObjectClass(javaNetworkOutput);
    methodSenddata = env->GetMethodID(cls, "senddata", "(B)V");
}

void KotlinNetworkOutputWrapper::SendData(uint8_t data) {
    JNIEnv* env = GetJNIEnv();
    env->CallVoidMethod(javaNetworkOutput, methodSenddata, static_cast<jbyte>(data));
}
