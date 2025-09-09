package com.retrogbm.models

import androidx.lifecycle.ViewModel
import com.retrogbm.EmulatorWrapper
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

class Emulator {
    companion object {
        init {
            System.loadLibrary("retrogbm")
        }

        var emulator: EmulatorWrapper = EmulatorWrapper()
    }
}

class EmulatorViewModel : ViewModel() {

    val emulator get() = Emulator.emulator

    private var tickJob: Job? = null

    fun startEmulator() {
        if (tickJob?.isActive == true)
            return

        // Background thread which the emulator runs on
        tickJob = CoroutineScope(Dispatchers.Default).launch {
            while (Emulator.emulator.isRunning()) {
                Emulator.emulator.tick()
            }
        }
    }

    fun stopEmulator() {
        tickJob?.cancel()
        Emulator.emulator.stop()
    }

    override fun onCleared() {
        super.onCleared()
        tickJob?.cancel()
    }

    fun isActive(): Boolean {
        return tickJob?.isActive == true
    }
}