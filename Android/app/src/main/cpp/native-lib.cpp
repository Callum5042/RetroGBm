#include <jni.h>
#include <string>
#include <fstream>

#include <Emulator.h>
#include <Cartridge.h>
#include <Ppu.h>
#include <Joypad.h>

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
Java_com_retrogbm_MainActivity_cartridgeGetTitle(JNIEnv *env, jobject thiz, jlong emulator_ptr) {

//    Cartridge* cartridge = reinterpret_cast<Cartridge*>(cartridge_ptr);
//    return env->NewStringUTF(cartridge->GetCartridgeInfo()->title.c_str());

    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    std::string title = emulator->GetCartridge()->GetCartridgeInfo()->title;
    return env->NewStringUTF(title.c_str());
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_MainActivity_createEmulator(JNIEnv *env, jobject thiz) {
    return reinterpret_cast<jlong>(new Emulator());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_retrogbm_MainActivity_loadRom(JNIEnv *env, jobject thiz, jlong emulator_ptr,
                                       jstring path) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    emulator->LoadRom(env->GetStringUTFChars(path, nullptr));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_retrogbm_MainActivity_tick(JNIEnv *env, jobject thiz, jlong emulator_ptr) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    emulator->Tick();
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_retrogbm_MainActivity_getPixels(JNIEnv *env, jobject thiz, jint colour) {
    int alpha = (colour >> 24) & 0xFF;
    int red = (colour >> 16) & 0xFF;
    int green = (colour >> 8) & 0xFF;
    int blue = (colour >> 0) & 0xFF;

    int swapped = (alpha << 24) | (blue << 16) | (green << 8) | (red);

    // Fill with red
    std::vector<uint32_t> pixels(160*144);
    std::fill(pixels.begin(), pixels.end(), static_cast<int>(swapped));

    // Copy the C++ array to the JVM array
    jintArray result = env->NewIntArray(pixels.size());
    env->SetIntArrayRegion(result, 0, pixels.size(), (jint*)pixels.data());
    return result;
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_retrogbm_MainActivity_getVideoBuffer(JNIEnv *env, jobject thiz, jlong emulator_ptr) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);

    size_t size = emulator->GetPpu()->GetContext()->video_buffer.size();
    // data = emulator->GetPpu()->GetContext()->video_buffer.data();

    // Copy the C++ array to the JVM array
    jintArray result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, (jint*)emulator->GetPpu()->GetContext()->video_buffer.data());
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_retrogbm_MainActivity_pressButton(JNIEnv *env, jobject thiz, jlong emulator_ptr, jint button, jboolean state) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    emulator->GetJoypad()->SetJoypad(static_cast<JoypadButton>(button), state);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_retrogbm_MainActivity_getFrameCount(JNIEnv *env, jobject thiz, jlong emulator_ptr) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    return emulator->GetFrameCount();
}