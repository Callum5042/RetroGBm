#include <jni.h>
#include <string>
#include <fstream>

#include <Cartridge.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_retrogbm_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_retrogbm_MainActivity_stringFromFile(JNIEnv *env, jobject thiz, jstring path)
{
    const char* str = env->GetStringUTFChars(path, nullptr);
    std::string strFromJava(str);
    env->ReleaseStringUTFChars(path, str);

    std::ifstream file(strFromJava);
    if (!file.is_open())
    {
        return env->NewStringUTF("No luck");
    }

    std::string line;
    std::getline(file, line);
    return env->NewStringUTF(line.c_str());
}
extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_MainActivity_createAndLoadCartridge(JNIEnv *env, jobject thiz, jstring path) {
    // TODO: implement createAndLoadCartridge()

    std::string cartridge_path(env->GetStringUTFChars(path, nullptr));

    Cartridge* cartridge = new Cartridge();
    cartridge->Load(cartridge_path);

    return reinterpret_cast<jlong>(cartridge);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_retrogbm_MainActivity_cartridgeGetTitle(JNIEnv *env, jobject thiz, jlong cartridge_ptr) {

    Cartridge* cartridge = reinterpret_cast<Cartridge*>(cartridge_ptr);
    return env->NewStringUTF(cartridge->GetCartridgeInfo()->title.c_str());
}