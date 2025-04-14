#pragma once

#include "RetroGBm/Audio/ISoundOutput.h"

#include <xaudio2.h>

#include <cstdint>
#include <vector>
#include <memory>

class XAudio2Output : public ISoundOutput, private IXAudio2VoiceCallback
{
public:
    XAudio2Output();
    virtual ~XAudio2Output();

    void Start() override;
    void Stop() override;
    void Play(int left, int right) override;

private:
    const int BufferSize = 1024;
    const int SampleRate = 22050;

    std::vector<uint8_t> m_EmptyBuffer;

    // Things
    std::vector<uint8_t> m_AudioBuffer;
    int m_SampleCount = 0;
    int _tick = 0;
    int _divider = 0;

    // XAudio2
    IXAudio2* m_Audio = nullptr;
    IXAudio2MasteringVoice* m_MasteringVoice = nullptr;
    IXAudio2SourceVoice* m_SourceVoice = nullptr;

    void Initialise();
    void QueueAudio(const std::vector<uint8_t>& buffer);

    // Inherited via IXAudio2VoiceCallback
    void __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
    void __stdcall OnVoiceProcessingPassEnd(void) override;
    void __stdcall OnStreamEnd(void) override;
    void __stdcall OnBufferStart(void* pBufferContext) override;
    void __stdcall OnBufferEnd(void* pBufferContext) override;
    void __stdcall OnLoopEnd(void* pBufferContext) override;
    void __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) override;
};