#include "RetroGBm/Pch.h"
#include "Audio/XAudio2Output.h"
#include "RetroGBm/Logger.h"

namespace
{
	struct AudioBuffer
	{
		std::vector<uint8_t> buffer;
	};
}

XAudio2Output::XAudio2Output()
{
	const int CPU_FREQUENCY = 4 * 1024 * 1024;
	_divider = CPU_FREQUENCY / SampleRate;

	m_AudioBuffer.resize(BufferSize, 128);
	m_EmptyBuffer.resize(SampleRate, 128);

	this->Initialise();
}

XAudio2Output::~XAudio2Output()
{
	if (m_SourceVoice)
	{
		m_SourceVoice->DestroyVoice();
	}

	if (m_MasteringVoice)
	{
		m_MasteringVoice->DestroyVoice();
	}

	if (m_Audio)
	{
		m_Audio->StopEngine();
		m_Audio->Release();
	}
}

void XAudio2Output::Start()
{
}

void XAudio2Output::Stop()
{
}

void XAudio2Output::Play(int left, int right)
{
	// Only want to add sample to the buffer synced with CPU
	if (_tick++ != 0)
	{
		_tick %= _divider;
		return;
	}

	// Populate buffer
	m_AudioBuffer[m_SampleCount++] = static_cast<uint8_t>(left);
	m_AudioBuffer[m_SampleCount++] = static_cast<uint8_t>(right);

	// Queue audio buffer
	if (m_SampleCount >= BufferSize)
	{
		this->QueueAudio(m_AudioBuffer);
		m_SampleCount = 0;
	}
}

void XAudio2Output::Initialise()
{
	HRESULT hr = S_OK;

	// Create audio engine
	hr = XAudio2Create(&m_Audio);
	if (FAILED(hr))
	{
		Logger::Error("XAudio2Create failed");
		return;
	}

	// Create mastering voice
	hr = m_Audio->CreateMasteringVoice(&m_MasteringVoice);
	if (FAILED(hr))
	{
		Logger::Error("IXAudio2::CreateMasteringVoice failed");
		return;
	}

	// Setup the format for the audio stream (stereo, 8-bit unsigned PCM)
	WAVEFORMATEX format = {};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = SampleRate;
	format.wBitsPerSample = 8;
	format.nBlockAlign = format.nChannels * (format.wBitsPerSample / 8);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	// Create the source voice (this is where audio data will be fed into)
	hr = m_Audio->CreateSourceVoice(&m_SourceVoice, &format, 0, 1.0f, this);
	if (FAILED(hr))
	{
		Logger::Error("IXAudio2::CreateSourceVoice failed");
		return;
	}

	m_SourceVoice->Start(0);
}

void XAudio2Output::QueueAudio(const std::vector<uint8_t>& buffer)
{
	AudioBuffer* audio_buffer = new AudioBuffer();
	audio_buffer->buffer = buffer;

	// Prepare XAudio2 buffer (8-bit unsigned PCM data, stereo interleaved)
	XAUDIO2_BUFFER xaudioBuffer = {};
	xaudioBuffer.pAudioData = audio_buffer->buffer.data();
	xaudioBuffer.AudioBytes = static_cast<UINT32>(audio_buffer->buffer.size());
	xaudioBuffer.Flags = 0;
	xaudioBuffer.pContext = audio_buffer;

	// Submit buffer for playback
	HRESULT hr = m_SourceVoice->SubmitSourceBuffer(&xaudioBuffer);
	if (FAILED(hr))
	{
		Logger::Error("IXAudio2SourceVoice::SubmitSourceBuffer failed");
	}
}

void __stdcall XAudio2Output::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
}

void __stdcall XAudio2Output::OnVoiceProcessingPassEnd(void)
{
}

void __stdcall XAudio2Output::OnStreamEnd(void)
{
}

void __stdcall XAudio2Output::OnBufferStart(void* pBufferContext)
{
}

void __stdcall XAudio2Output::OnBufferEnd(void* pBufferContext)
{
	AudioBuffer* buffer = reinterpret_cast<AudioBuffer*>(pBufferContext);
	if (buffer != nullptr)
	{
		delete buffer;
		buffer = nullptr;
	}

	// Bit of a hack - needs improving
	XAUDIO2_VOICE_STATE voiceState;
	m_SourceVoice->GetState(&voiceState);

	if (voiceState.BuffersQueued == 0)
	{
		this->QueueAudio(m_EmptyBuffer);
	}
}

void __stdcall XAudio2Output::OnLoopEnd(void* pBufferContext)
{
}

void __stdcall XAudio2Output::OnVoiceError(void* pBufferContext, HRESULT Error)
{
}
