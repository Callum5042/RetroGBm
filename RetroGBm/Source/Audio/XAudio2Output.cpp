#include "RetroGBm/Pch.h"
#include "Audio/XAudio2Output.h"
#include "RetroGBm/Logger.h"
#include "RetroGBm/Constants.h"

XAudio2Output::XAudio2Output()
{
	m_Divider = GameBoy::TicksPerSec / SampleRate;

	for (int i = 0; i < AudioBufferCount; ++i)
	{
		m_RingBuffers[i].buffer.resize(SamplesPerBuffer * 2, 128);
		m_RingBuffers[i].inUse = false;
	}

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

void XAudio2Output::SetFrequencyRatio(float ratio)
{
	m_SourceVoice->SetFrequencyRatio(ratio);
}

void XAudio2Output::Play(int left, int right)
{
	// Only want to add sample to the buffer synced with CPU
	if (m_Tick++ != 0)
	{
		m_Tick %= m_Divider;
		return;
	}

	if (!EnableAudio)
	{
		return;
	}

	// Get current buffer
	AudioBuffer& buf = m_RingBuffers[m_CurrentBuffer];

	// Skip if all buffers are full (avoid overwriting submitted ones)
	if (buf.inUse)
		return;

	// Fill current buffer
	buf.buffer[m_CurrentSampleOffset++] = static_cast<uint8_t>(left);
	buf.buffer[m_CurrentSampleOffset++] = static_cast<uint8_t>(right);

	// Buffer full, submit to XAudio2
	if (m_CurrentSampleOffset >= buf.buffer.size())
	{
		XAUDIO2_BUFFER xaudioBuffer = {};
		xaudioBuffer.pAudioData = buf.buffer.data();
		xaudioBuffer.AudioBytes = static_cast<UINT32>(buf.buffer.size());
		xaudioBuffer.pContext = &buf;

		HRESULT hr = m_SourceVoice->SubmitSourceBuffer(&xaudioBuffer);
		if (FAILED(hr))
		{
			Logger::Error("SubmitSourceBuffer failed");
		}
		else
		{
			buf.inUse = true;
		}

		// Move to next buffer
		m_CurrentBuffer = (m_CurrentBuffer + 1) % AudioBufferCount;
		m_CurrentSampleOffset = 0;
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
	if (pBufferContext != nullptr)
	{
		AudioBuffer* buffer = reinterpret_cast<AudioBuffer*>(pBufferContext);
		buffer->inUse = false;
	}
}

void __stdcall XAudio2Output::OnLoopEnd(void* pBufferContext)
{
}

void __stdcall XAudio2Output::OnVoiceError(void* pBufferContext, HRESULT Error)
{
}
