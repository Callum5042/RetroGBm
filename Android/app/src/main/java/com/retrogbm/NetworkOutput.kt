package com.retrogbm

class NetworkOutput {
    val nativePtr: Long
    private external fun nativeCreate(): Long

    init {
        nativePtr = nativeCreate()
    }

    fun senddata(data: Byte) {
        onSendDataCallback?.invoke(data)
    }

    fun onSendData(callback: (data: Byte) -> Unit) {
        onSendDataCallback = callback
    }

    private var onSendDataCallback: ((data: Byte) -> Unit)? = null
}