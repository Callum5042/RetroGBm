#include <jni.h>
#include <string>
#include <fstream>

#include <Emulator.h>
#include <Cartridge.h>
#include <Ppu.h>
#include <Joypad.h>

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

        size_t size = emulator->GetPpu()->GetContext()->video_buffer.size();
        // data = emulator->GetPpu()->GetContext()->video_buffer.data();

        // Copy the C++ array to the JVM array
        jintArray result = env->NewIntArray(size);
        env->SetIntArrayRegion(result, 0, size, (jint*)emulator->GetPpu()->GetContext()->video_buffer.data());
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

        std::string title = emulator->GetCartridge()->GetCartridgeInfo()->title;
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
}