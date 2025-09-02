package com.retrogbm

enum class JoyPadButton(val buttonCode: Int) {
    A(0),
    B(1),
    Start(2),
    Select(3),
    Up(4),
    Down(5),
    Left(6),
    Right(7)
}

data class CheatCode(
    var name: String,
    var code: Array<String>,
    var enabled: Boolean
)

class EmulatorWrapper {

    val displayOutput: DisplayOutput = DisplayOutput()
    val soundOutput: SoundOutput = SoundOutput()

    private var emulatorPtr: Long = 0

    fun loadRom(data: ByteArray, path: String, skipBootRom: Boolean) {
        emulatorPtr = createEmulator(displayOutput.nativePtr, soundOutput.nativePtr)
        setBatteryPath(emulatorPtr, path)
        setBootRom(emulatorPtr, !skipBootRom)
        loadRomFromByteArray(emulatorPtr, data)
    }

    fun tick() {
        tick(emulatorPtr)
    }

    fun getVideoBuffer(): IntArray {
        return getVideoBuffer(emulatorPtr)
    }

    fun pressButton(button: JoyPadButton, state: Boolean) {
        pressButton(emulatorPtr, button.buttonCode, state)
    }

    fun setEmulationSpeed(speedMultiplier: Float) {
        setEmulationSpeedMultiplier(emulatorPtr, speedMultiplier)
    }

    fun getCartridgeTitle(): String {
        return cartridgeGetTitle(emulatorPtr)
    }

    fun stop() {
        stop(emulatorPtr)
    }

    fun isRunning(): Boolean {
        return isRunning(emulatorPtr)
    }

    fun saveState(path: String) {
        saveState(emulatorPtr, path)
    }

    fun loadState(path: String) {
        loadState(emulatorPtr, path)
    }

    fun pause() {
        pause(emulatorPtr)
    }

    fun resume() {
        resume(emulatorPtr)
    }

    fun getCheatCodes(): Array<CheatCode> {
        return getCheatCodes(emulatorPtr)
    }

    fun setCheatCodes(codes: Array<CheatCode>) {
        setCheatCodes(emulatorPtr, codes)
    }

    private external fun createEmulator(displayOutputPtr: Long, soundOutputPtr: Long): Long
    private external fun loadRom(emulatorPtr: Long, path: String)
    private external fun loadRomFromByteArray(emulatorPtr: Long, data: ByteArray)
    private external fun tick(emulatorPtr: Long)
    private external fun stop(emulatorPtr: Long)
    private external fun isRunning(emulatorPtr: Long): Boolean

    private external fun getVideoBuffer(emulatorPtr: Long): IntArray
    private external fun pressButton(emulatorPtr: Long, button: Int, state: Boolean)

    private external fun cartridgeGetTitle(emulatorPtr: Long): String

    private external fun saveState(emulatorPtr: Long, path: String)
    private external fun loadState(emulatorPtr: Long, path: String)

    private external fun setBatteryPath(emulatorPtr: Long, path: String)

    private external fun pause(emulatorPtr: Long)
    private external fun resume(emulatorPtr: Long)
    private external fun setEmulationSpeedMultiplier(emulatorPtr: Long, speedMultiplier: Float)

    private external fun getCheatCodes(emulatorPtr: Long): Array<CheatCode>
    private external fun setCheatCodes(emulatorPtr: Long, codes: Array<CheatCode>)

    private external fun setBootRom(emulatorPtr: Long, enabled: Boolean)
}