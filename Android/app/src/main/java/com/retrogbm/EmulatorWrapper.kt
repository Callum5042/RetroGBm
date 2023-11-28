package com.retrogbm

class EmulatorWrapper {

    private var emulatorPtr: Long = 0

    fun loadRom(path: String) {
        emulatorPtr = createEmulator()
        loadRom(emulatorPtr, path)
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



    private external fun createEmulator(): Long
    private external fun loadRom(emulatorPtr: Long, path: String)
    private external fun tick(emulatorPtr: Long)

    private external fun getVideoBuffer(emulatorPtr: Long): IntArray
    private external fun pressButton(emulatorPtr: Long, button: Int, state: Boolean)

    private external fun cartridgeGetTitle(emulatorPtr: Long): String
}