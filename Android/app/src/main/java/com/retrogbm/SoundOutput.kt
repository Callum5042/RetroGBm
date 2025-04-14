package com.retrogbm

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import kotlin.math.sin

class SoundOutput {
    val nativePtr: Long
    private external fun nativeCreate(): Long

    init {
        nativePtr = nativeCreate()
    }

    private val sampleRate = 22050
    private val bufferSize = 1024

    fun start() {
        audioTrack.play()
    }

    fun stop() {
        audioTrack.stop()
    }

    fun playBufferedSamples(samples: ByteArray) {
        if (this.audioOn) {
            audioTrack.write(samples, 0, samples.size)
        }
    }

    fun toggleAudio(audioOn: Boolean) {
        this.audioOn = audioOn
    }

    private var audioOn = true;

    private val audioTrack = AudioTrack(
        AudioManager.STREAM_MUSIC,
        sampleRate,
        AudioFormat.CHANNEL_OUT_STEREO,
        AudioFormat.ENCODING_PCM_8BIT,
        bufferSize,
        AudioTrack.MODE_STREAM
    )
}
