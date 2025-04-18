package com.retrogbm

import android.util.Log

class DisplayOutput {
    val nativePtr: Long
    private external fun nativeCreate(): Long

    init {
        nativePtr = nativeCreate()
    }

    fun draw(pixels: IntArray) {
        Log.d("Display", "Display")
        onDrawCallback?.invoke(pixels)
    }

    fun onDraw(callback: (pixels: IntArray) -> Unit) {
        onDrawCallback = callback
    }

    private var onDrawCallback: ((samples: IntArray) -> Unit)? = null
}