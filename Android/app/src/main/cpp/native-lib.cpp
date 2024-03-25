#include <jni.h>
#include <string>
#include <fstream>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/PixelProcessor.h>
#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/Display.h>

extern "C"
{
    JNIEXPORT jlong JNICALL
    Java_com_retrogbm_EmulatorWrapper_createEmulator(JNIEnv *env, jobject thiz)
    {
        return reinterpret_cast<jlong>(new Emulator());
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
            emulator->LoadState(env->GetStringUTFChars(path, nullptr));
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
}
