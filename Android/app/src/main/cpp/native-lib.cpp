#include <jni.h>
#include <string>
#include <fstream>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/Logger.h>

#include "KotlinSoundOutputWrapper.h"

//class NullSoundOutput : public ISoundOutput
//{
//public:
//    NullSoundOutput() = default;
//
//    void Start() override {
//
//    }
//
//    void Stop() override {
//
//    }
//
//    void Play(int left, int right) override {
//
//    }
//};

extern "C"
{
    JNIEXPORT jlong JNICALL
    Java_com_retrogbm_EmulatorWrapper_createEmulator(JNIEnv *env, jobject thiz, jlong soundOutputPtr)
    {
        ISoundOutput* sound_output = reinterpret_cast<ISoundOutput*>(soundOutputPtr);

        // ISoundOutput* sound_output = new NullSoundOutput();
        return reinterpret_cast<jlong>(new Emulator(sound_output));
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_loadRom(JNIEnv *env, jobject thiz, jlong emulator_ptr, jstring path)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->LoadRom(env->GetStringUTFChars(path, nullptr));
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_tick(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->Tick();
        }
    }

    JNIEXPORT jintArray JNICALL
    Java_com_retrogbm_EmulatorWrapper_getVideoBuffer(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator == nullptr)
        {
            // Fill with red
            std::vector<uint32_t> pixels(160*144);
            std::fill(pixels.begin(), pixels.end(), static_cast<int>(0xFFFF0000));

            // Copy the C++ array to the JVM array
            jintArray result = env->NewIntArray(pixels.size());
            env->SetIntArrayRegion(result, 0, pixels.size(), (jint*)pixels.data());
            return result;
        }

        size_t size = emulator->GetDisplay()->GetVideoBufferSize();
        // data = emulator->GetPpu()->GetContext()->video_buffer.data();

        // Copy the C++ array to the JVM array
        jintArray result = env->NewIntArray(size);
        env->SetIntArrayRegion(result, 0, size, (jint*)emulator->GetDisplay()->GetVideoBuffer());
        return result;
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_pressButton(JNIEnv *env, jobject thiz, jlong emulator_ptr, jint button, jboolean state) {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->GetJoypad()->SetJoypad(static_cast<JoypadButton>(button), state);
        }
    }

    JNIEXPORT jstring JNICALL
    Java_com_retrogbm_EmulatorWrapper_cartridgeGetTitle(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator == nullptr)
        {
            return env->NewStringUTF("Emulator not loaded");
        }

        std::string title = emulator->GetCartridge()->GetCartridgeData().title;
        return env->NewStringUTF(title.c_str());
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_stop(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->Stop();
        }
    }

    JNIEXPORT jboolean JNICALL
    Java_com_retrogbm_EmulatorWrapper_isRunning(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            return emulator->IsRunning();
        }

        return false;
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_loadRomFromByteArray(JNIEnv *env, jobject thiz, jlong emulator_ptr, jbyteArray data)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            jsize length = env->GetArrayLength(data);
            jbyte* byteArrayElements = env->GetByteArrayElements(data, nullptr);

            std::vector<uint8_t> result(byteArrayElements, byteArrayElements + length);
            env->ReleaseByteArrayElements(data, byteArrayElements, JNI_ABORT);

            emulator->LoadRom(result);
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_saveState(JNIEnv *env, jobject thiz, jlong emulator_ptr, jstring path)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->SaveState(env->GetStringUTFChars(path, nullptr));
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_loadState(JNIEnv *env, jobject thiz, jlong emulator_ptr, jstring path)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            try
            {
                emulator->LoadState(env->GetStringUTFChars(path, nullptr));
            }
            catch (const std::exception& ex)
            {
                env->ThrowNew(env->FindClass("java/lang/RuntimeException"), ex.what());
            }
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_setBatteryPath(JNIEnv *env, jobject thiz, jlong emulator_ptr, jstring path)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->SetBatteryPath(env->GetStringUTFChars(path, nullptr));
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_pause(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->Pause(true);
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_resume(JNIEnv *env, jobject thiz, jlong emulator_ptr)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->Pause(false);
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_setEmulationSpeedMultiplier(JNIEnv *env, jobject thiz, jlong emulator_ptr, jfloat speed)
    {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        if (emulator != nullptr)
        {
            emulator->SetEmulationSpeedMultipler(speed);
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_LoggerWrapper_initializeLogger(JNIEnv *env, jobject thiz) {
        Logger::Initialise(LogLevel::LOG_INFO);
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_LoggerWrapper_info(JNIEnv *env, jobject thi, jstring message) {
        Logger::Info(env->GetStringUTFChars(message, nullptr));
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_LoggerWrapper_waring(JNIEnv *env, jobject thi, jstring message) {
        Logger::Warning(env->GetStringUTFChars(message, nullptr));
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_LoggerWrapper_error(JNIEnv *env, jobject thi, jstring message) {
        Logger::Error(env->GetStringUTFChars(message, nullptr));
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_LoggerWrapper_critical(JNIEnv *env, jobject thi, jstring message) {
        Logger::Critical(env->GetStringUTFChars(message, nullptr));
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_SoundOutput_nativeCreate(JNIEnv *env, jobject thiz) {
    ISoundOutput* output = new KotlinSoundOutputWrapper(env, thiz);
    return reinterpret_cast<jlong>(output);
}