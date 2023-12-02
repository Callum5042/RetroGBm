package com.retrogbm

class EmulatorWrapper {

    private var emulatorPtr: Long = 0

    fun loadRom(path: String, batteryPath: String) {
        emulatorPtr = createEmulator()
        setBatteryPath(emulatorPtr, batteryPath)
        loadRom(emulatorPtr, path)
    }

    fun loadRom(data: ByteArray, path: String) {
        emulatorPtr = createEmulator()
        setBatteryPath(emulatorPtr, path)
        loadRomFromByteArray(emulatorPtr, data)
    }

    fun tick() {
        tick(emulatorPtr)
    }

    fun getVideoBuffer(): IntArray {
        return getVideoBuffer(emulatorPtr)
    }

    fun pressButton(button: Int, state: Boolean) {
        pressButton(emulatorPtr, button, state)
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

    private external fun createEmulator(): Long
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
}