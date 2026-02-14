#include <jni.h>
#include <string>
#include <fstream>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cartridge/BaseCartridge.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/Logger.h>

#include "KotlinSoundOutputWrapper.h"
#include "KotlinDisplayOutputWrapper.h"
#include "KotlinNetworkOutputWrapper.h"

extern "C"
{
    JNIEXPORT jlong JNICALL
    Java_com_retrogbm_EmulatorWrapper_createEmulator(JNIEnv *env,
                                                     jobject thiz,
                                                     jlong displayOutputPtr,
                                                     jlong soundOutputPtr,
                                                     jlong networkOutputPtr)
    {
        IDisplayOutput* display_output = reinterpret_cast<IDisplayOutput*>(displayOutputPtr);
        ISoundOutput* sound_output = reinterpret_cast<ISoundOutput*>(soundOutputPtr);
        INetworkOutput* network_output = reinterpret_cast<INetworkOutput*>(networkOutputPtr);

        return reinterpret_cast<jlong>(new Emulator(display_output, sound_output, network_output));
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

    JNIEXPORT jobjectArray JNICALL
    Java_com_retrogbm_EmulatorWrapper_getCheatCodes(JNIEnv* env, jobject thiz, jlong emulator_ptr) {

        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);

        // Get C++ vector
        std::vector<CheatCode> codes = emulator->GetGamesharkCodes();

        // Find Kotlin CheatCode class
        jclass cheatCodeClass = env->FindClass("com/retrogbm/CheatCode");
        if (!cheatCodeClass) return nullptr;

        // Get constructor: CheatCode(String name, String[] code, boolean enabled)
        jmethodID ctor = env->GetMethodID(cheatCodeClass, "<init>",
                                          "(Ljava/lang/String;[Ljava/lang/String;Z)V");
        if (!ctor) return nullptr;

        // Prepare return array
        jobjectArray result = env->NewObjectArray((jsize)codes.size(), cheatCodeClass, nullptr);

        for (jsize i = 0; i < (jsize)codes.size(); i++) {
            // Name
            jstring name = env->NewStringUTF(codes[i].name.c_str());

            // Code array
            jclass stringClass = env->FindClass("java/lang/String");
            jobjectArray codeArray = env->NewObjectArray((jsize)codes[i].code.size(), stringClass, nullptr);
            for (jsize j = 0; j < (jsize)codes[i].code.size(); j++) {
                env->SetObjectArrayElement(codeArray, j, env->NewStringUTF(codes[i].code[j].c_str()));
            }

            // Enabled flag
            jboolean enabled = (jboolean)codes[i].enabled;

            // Create Kotlin CheatCode object
            jobject cheatObj = env->NewObject(cheatCodeClass, ctor, name, codeArray, enabled);

            // Set into return array
            env->SetObjectArrayElement(result, i, cheatObj);

            // Cleanup local refs
            env->DeleteLocalRef(name);
            env->DeleteLocalRef(codeArray);
            env->DeleteLocalRef(cheatObj);
        }

        return result;
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_setCheatCodes(JNIEnv* env, jobject thiz, jlong emulator_ptr, jobjectArray cheatCodes) {
        jsize count = env->GetArrayLength(cheatCodes);
        std::vector<CheatCode> cppCodes;
        cppCodes.reserve(count);

        jclass cheatCodeClass = env->FindClass("com/retrogbm/CheatCode");
        jfieldID fidName = env->GetFieldID(cheatCodeClass, "name", "Ljava/lang/String;");
        jfieldID fidCode = env->GetFieldID(cheatCodeClass, "code", "[Ljava/lang/String;");
        jfieldID fidEnabled = env->GetFieldID(cheatCodeClass, "enabled", "Z");

        for (jsize i = 0; i < count; i++) {
            jobject cheatObj = env->GetObjectArrayElement(cheatCodes, i);

            // Read name
            jstring jName = (jstring)env->GetObjectField(cheatObj, fidName);
            const char* cname = env->GetStringUTFChars(jName, nullptr);

            // Read codes array
            jobjectArray jCodeArray = (jobjectArray)env->GetObjectField(cheatObj, fidCode);
            jsize codeCount = env->GetArrayLength(jCodeArray);
            std::vector<std::string> codeVec;
            codeVec.reserve(codeCount);
            for (jsize j = 0; j < codeCount; j++) {
                jstring jCodeStr = (jstring)env->GetObjectArrayElement(jCodeArray, j);
                const char* ccode = env->GetStringUTFChars(jCodeStr, nullptr);
                codeVec.emplace_back(ccode);
                env->ReleaseStringUTFChars(jCodeStr, ccode);
                env->DeleteLocalRef(jCodeStr);
            }

            // Read enabled flag
            jboolean enabled = env->GetBooleanField(cheatObj, fidEnabled);

            // Push to C++ vector
            CheatCode cc;
            cc.name = cname;
            cc.code = std::move(codeVec);
            cc.enabled = (bool)enabled;
            cppCodes.push_back(std::move(cc));

            // Cleanup
            env->ReleaseStringUTFChars(jName, cname);
            env->DeleteLocalRef(jName);
            env->DeleteLocalRef(jCodeArray);
            env->DeleteLocalRef(cheatObj);

            Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
            emulator->SetGamesharkCodes(cppCodes);
        }
    }

    JNIEXPORT void JNICALL
    Java_com_retrogbm_EmulatorWrapper_setBootRom(JNIEnv *env, jobject thiz, jlong emulator_ptr, jboolean enabled) {
        Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
        emulator->SetBootRom(enabled);
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_SoundOutput_nativeCreate(JNIEnv *env, jobject thiz) {
    ISoundOutput* output = new KotlinSoundOutputWrapper(env, thiz);
    return reinterpret_cast<jlong>(output);
}
extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_DisplayOutput_nativeCreate(JNIEnv *env, jobject thiz) {
    IDisplayOutput* output = new KotlinDisplayOutputWrapper(env, thiz);
    return reinterpret_cast<jlong>(output);
}
extern "C"
JNIEXPORT jlong JNICALL
Java_com_retrogbm_NetworkOutput_nativeCreate(JNIEnv *env, jobject thiz) {
    INetworkOutput* output = new KotlinNetworkOutputWrapper(env, thiz);
    return reinterpret_cast<jlong>(output);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_retrogbm_EmulatorWrapper_linkCableData(JNIEnv *env, jobject thiz, jlong emulator_ptr,
                                                jbyte data) {
    Emulator* emulator = reinterpret_cast<Emulator*>(emulator_ptr);
    emulator->LinkCableData(data);
}